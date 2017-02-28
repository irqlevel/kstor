#pragma once

#include "error.h"
#include "memory.h"
#include "page.h"
#include "kapi.h"
#include "block_device_interface.h"
#include "event.h"
#include "list.h"
#include "shared_ptr.h"
#include "atomic.h"

namespace Core
{

template<Memory::PoolType PoolType>
class Bio
{
public:
    typedef SharedPtr<Bio<PoolType>, PoolType> Ptr;

public:
    Bio(int pageCount, Error& err)
        : BioPtr(nullptr)
        , PageCount(pageCount)
        , IoError(Error::NotExecuted)
        , PostEndIoHandler(nullptr)
        , PostEndIoCtx(nullptr)
    {
        if (!err.Ok())
        {
            return;
        }

        if (pageCount == 0)
        {
            err.SetInvalidValue();
            return;
        }

        BioPtr = get_kapi()->alloc_bio(pageCount, get_kapi_pool_type(PoolType));
        if (!BioPtr)
        {
            trace(0, "Can't allocate bio");
            err.SetNoMemory();
            return;
        }

        int rc = get_kapi()->set_bio_end_io(BioPtr, &Bio::EndIo, this);
        if (rc)
        {
            trace(0, "Can't set bio private");
            get_kapi()->free_bio(BioPtr);
            BioPtr = nullptr;
            err = Error(rc);
            return;
        }

        trace(4, "Bio 0x%p bio 0x%p ctor", this, BioPtr);
    }

    Bio(BlockDeviceInterface& blockDevice, const typename Page<PoolType>::Ptr& page, unsigned long long sector,
        Error& err, bool write, bool flush = false, bool fua = false, int offset = 0, int len = 0)
        : Bio(1, err)
    {
        if (!err.Ok())
        {
            return;
        }

        err = SetPage(0, page, offset, (len == 0) ? page->GetSize() : len);
        if (!err.Ok())
        {
            return;
        }

        if (write)
        {
            SetWrite();
            if (fua)
            {
                SetFua();
            }
            if (flush)
            {
                SetFlush();
            }
        }
        else
        {
            SetRead();
        }
        SetPosition(sector);
        SetBdev(blockDevice);
    }

    void SetBdev(BlockDeviceInterface& blockDevice)
    {
        get_kapi()->set_bio_bdev(BioPtr, blockDevice.GetBdev());
    }

    void SetRead()
    {
        get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_READ);
    }

    void SetWrite()
    {
        get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_WRITE);
    }

    void SetFua()
    {
        get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_FUA);
    }

    void SetFlush()
    {
        get_kapi()->set_bio_rw(BioPtr, KAPI_BIO_FLUSH);
    }

    Error SetPage(int pageIndex, const typename Page<PoolType>::Ptr& page, int offset, int len)
    {
        if (!PageList.AddTail(page))
            return Error::NoMemory;

        int rc = get_kapi()->set_bio_page(BioPtr, pageIndex, page->GetPagePtr(), offset, len);
        if (rc)
        {
            trace(0, "Can't set bio page offset %d len %d, rc %d", offset, len, rc);
            PageList.PopTail();
        }
        return Error(rc);
    }


    void Wait()
    {
        EndIoEvent.Wait();
    }

    void SetPosition(unsigned long long sector)
    {
        get_kapi()->set_bio_position(BioPtr, sector);
    }

    void Submit()
    {
        trace(4, "Bio 0x%p bio 0x%p submit", this, BioPtr);
        get_kapi()->submit_bio(BioPtr);
    }

    Error GetError()
    {
        return IoError;
    }

    Error Exec()
    {
        Submit();
        Wait();
        return GetError();
    }

    virtual ~Bio()
    {
        trace(4, "Bio 0x%p bio 0x%p dtor", this, BioPtr);
        if (BioPtr != nullptr)
        {
            get_kapi()->free_bio(BioPtr);
            BioPtr = nullptr;
        }
    }

    static SharedPtr<Bio<PoolType>, PoolType> Create(BlockDeviceInterface& blockDevice,
        const typename Page<PoolType>::Ptr& page, unsigned long long sector, Error& err,
        bool write, bool flush = false, bool fua = false, int offset = 0, int len = 0)
    {
        SharedPtr<Bio<PoolType>, PoolType> bio =
            MakeShared<Bio<PoolType>, PoolType>(blockDevice, page, sector, err,
                                                write, flush, fua, offset, len);
        if (bio.Get() == nullptr)
        {
            err = Error::NoMemory;
        }
        return bio;
    }

    typedef void (*PostEndIoHandlerType)(Bio<PoolType>* bio, void* ctx);

    void SetPostEndIoHandler(PostEndIoHandlerType handler, void* ctx)
    {
        PostEndIoHandler = handler;
        PostEndIoCtx = ctx;
    }

private:
    Bio(const Bio& other) = delete;
    Bio(Bio&& other) = delete;
    Bio& operator=(const Bio& other) = delete;
    Bio& operator=(Bio&& other) = delete;

    void EndIo(int err)
    {
        trace(4, "Bio 0x%p bio 0x%p endio err %d", this, BioPtr, err);
        IoError.SetCode(err);
        EndIoEvent.SetAll();

        if (PostEndIoHandler != nullptr)
            PostEndIoHandler(this, PostEndIoCtx);
    }

    static void EndIo(void* bio, int err)
    {
        Bio* bio_ = static_cast<Bio*>(get_kapi()->get_bio_private(bio));
        bio_->EndIo(err);
    }

    void* BioPtr;
    int PageCount;
    Event EndIoEvent;
    Error IoError;
    LinkedList<typename Page<PoolType>::Ptr, PoolType> PageList;
    PostEndIoHandlerType PostEndIoHandler;
    void* PostEndIoCtx;
};

template<Memory::PoolType PoolType>
class BioList
{
public:
    typedef SharedPtr<BioList<PoolType>, PoolType> Ptr;
public:
    BioList(BlockDeviceInterface& blockDevice)
        : BlockDev(blockDevice)
    {
    }

    virtual ~BioList()
    {
    }

    Error AddIo(const typename Page<PoolType>::Ptr& page, unsigned long long position, bool write)
    {
        if (position & 511)
            return Error::InvalidValue;

        Error err;
        auto bio = Bio<PoolType>::Create(BlockDev, page, position / 512, err, write);
        if (!err.Ok())
        {
            return err;
        }

        if (!ReqList.AddTail(bio))
        {
            return Error::NoMemory;
        }

        return Error::Success;
    }

    void Submit(bool flushFua = false)
    {
        if (ReqList.IsEmpty())
            return;

        if (flushFua)
        {
            auto lastBio = ReqList.Tail();
            lastBio->SetFlush();
            lastBio->SetFua();
        }

        auto it = ReqList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            ReqCompleteCount.Inc();
        }

        it = ReqList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            auto bio = it.Get();
            bio->SetPostEndIoHandler(&BioList<PoolType>::PostEndIoHandler, this);
            bio->Submit();
        }
    }

    void Wait()
    {
        if (!ReqList.IsEmpty())
        {
            ReqCompleteEvent.Wait();
        }
    }

    Error GetResult()
    {
        return Result;
    }

    Error Exec(bool flushFua = false)
    {
        Submit();
        Wait();
        return GetResult();
    }

private:
    BioList(const BioList& other) = delete;
    BioList(BioList&& other) = delete;
    BioList& operator=(const BioList& other) = delete;
    BioList& operator=(BioList&& other) = delete;

    void PostEndIoHandler(Bio<PoolType>* bio)
    {
        auto err = bio->GetError();
        if (!err.Ok())
            Result = err;

        if (ReqCompleteCount.DecAndTest())
            ReqCompleteEvent.SetAll();
    }

    static void PostEndIoHandler(Bio<PoolType>* bio, void* ctx)
    {
        BioList<PoolType>* bioList = static_cast<BioList<PoolType>*>(ctx);
        bioList->PostEndIoHandler(bio);
    }

    LinkedList<typename Bio<PoolType>::Ptr, PoolType> ReqList;
    BlockDeviceInterface& BlockDev;
    Atomic ReqCompleteCount;
    Event ReqCompleteEvent;
    Error Result;
};

typedef Core::BioList<Core::Memory::PoolType::NoIO> NoIOBioList;

}