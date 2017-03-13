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
        , PostEndIoHandler(nullptr)
        , PostEndIoCtx(nullptr)
        , Write(false)
        , Flush(false)
        , Preflush(false)
        , Fua(false)
        , Sync(false)
        , Position(0)
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
        Error& err, bool write, size_t len = 0, size_t offset = 0,
        bool preflush = false, bool fua = false, bool sync = false)
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
            if (preflush)
            {
                SetPreflush();
            }
            if (sync)
            {
                SetSync();
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
        Write = false;
    }

    void SetWrite()
    {
        Write = true;
    }

    void SetFua()
    {
        Fua = true;
    }

    void SetFlush()
    {
        Flush = true;
    }

    void SetPreflush()
    {
        Preflush = true;
    }

    void SetSync()
    {
        Sync = true;
    }

    Error SetPage(int pageIndex, const typename Page<PoolType>::Ptr& page, size_t offset, size_t len)
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
        Position = sector;
        get_kapi()->set_bio_position(BioPtr, Position);
    }

    void Submit()
    {
        trace(4, "Bio 0x%p bio 0x%p submit pos %llu write %d flush %d fua %d sync %d",
            this, BioPtr, Position, Write, Flush, Fua, Sync);

        get_kapi()->submit_bio(BioPtr,
            (Flush) ? KAPI_BIO_OP_FLUSH :
            ((Write) ? KAPI_BIO_OP_WRITE : KAPI_BIO_OP_READ),
            ((Fua) ? KAPI_BIO_REQ_FUA : 0) |
            ((Sync) ? KAPI_BIO_REQ_SYNC : 0) |
            ((Preflush) ? KAPI_BIO_REQ_PREFLUSH : 0));
    }

    Error GetResult()
    {
        return Result;
    }

    Error SubmitWaitResult()
    {
        Submit();
        Wait();
        return GetResult();
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
        bool write, int len = 0, int offset = 0, bool preflush = false, bool fua = false, bool sync = false)
    {
        SharedPtr<Bio<PoolType>, PoolType> bio =
            MakeShared<Bio<PoolType>, PoolType>(blockDevice, page, sector, err,
                                                write, len, offset, preflush, fua, sync);
        if (bio.Get() == nullptr)
        {
            err = Error::NoMemory;
            return bio;
        }

        if (!err.Ok())
            bio.Reset();

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
        Result.SetCode(err);
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
    Error Result;
    LinkedList<typename Page<PoolType>::Ptr, PoolType> PageList;
    PostEndIoHandlerType PostEndIoHandler;
    void* PostEndIoCtx;
    bool Write;
    bool Flush;
    bool Preflush;
    bool Fua;
    bool Sync;
    unsigned long long Position;
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

    Error AddIo(const void *data, size_t dataSize, unsigned long long position, bool write)
    {
        Error err;
        auto page = Page<PoolType>::Create(err);
        if (!err.Ok())
            return err;

        if (dataSize > page->GetSize())
            return Core::Error::Overflow;

        auto size = page->Write(data, dataSize, 0);
        if (size != dataSize)
            return Core::Error::UnexpectedEOF;

        return AddIo(page, position, write, dataSize);
    }

    Error AddIo(const typename Page<PoolType>::Ptr& page, unsigned long long position, bool write,
        size_t len = 0)
    {
        if (position & 511)
            return Error::InvalidValue;

        Error err;
        auto bio = Bio<PoolType>::Create(BlockDev, page, position / 512, err, write, len);
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

    void SubmitWait(bool preflushFua = false)
    {
        if (ReqList.IsEmpty())
            return;

        if (!preflushFua)
        {
            Submit(ReqList, ReqList.Count());
            Wait();
            return;
        }

        auto lastBio = ReqList.Tail();
        lastBio->SetPreflush();
        lastBio->SetFua();
        lastBio->SetSync();

        size_t count = ReqList.Count();
        if (count > 1)
        {
            Submit(ReqList, count - 1);
            Wait();
            auto err = GetResult();
            if (!err.Ok())
            {
                return;
            }
        }

        Result = lastBio->SubmitWaitResult();
    }

    Error GetResult()
    {
        return Result;
    }

    Error SubmitWaitResult(bool preflushFua = false)
    {
        SubmitWait(preflushFua);
        return GetResult();
    }

    Error SubmitWaitResult(const typename Page<PoolType>::Ptr& page, unsigned long long position,
                bool write, bool preflushFua = false)
    {
        Error err = AddIo(page, position, write);
        if (!err.Ok())
            return err;

        return SubmitWaitResult(preflushFua);
    }

private:
    BioList(const BioList& other) = delete;
    BioList(BioList&& other) = delete;
    BioList& operator=(const BioList& other) = delete;
    BioList& operator=(BioList&& other) = delete;

    void PostEndIoHandler(Bio<PoolType>* bio)
    {
        auto err = bio->GetResult();
        if (!err.Ok() && Result.Ok())
            Result = err;

        if (ReqCompleteCount.DecAndTest())
            ReqCompleteEvent.SetAll();
    }

    static void PostEndIoHandler(Bio<PoolType>* bio, void* ctx)
    {
        BioList<PoolType>* bioList = static_cast<BioList<PoolType>*>(ctx);
        bioList->PostEndIoHandler(bio);
    }

    void Submit(LinkedList<typename Bio<PoolType>::Ptr, PoolType>& reqList, size_t maxCount)
    {
        ReqCompleteCount.Set(0);
        ReqCompleteEvent.Reset();

        size_t i = 0;
        auto it = reqList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            if (i >= maxCount)
                break;

            ReqCompleteCount.Inc();
            i++;
        }

        i = 0;
        it = reqList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            if (i >= maxCount)
                break;

            auto bio = it.Get();
            bio->SetPostEndIoHandler(&BioList<PoolType>::PostEndIoHandler, this);
            bio->Submit();
            i++;
        }
    }

    void Wait()
    {
        if (ReqCompleteCount.Get() != 0)
        {
            ReqCompleteEvent.Wait();
            ReqCompleteEvent.Reset();
        }
    }

    LinkedList<typename Bio<PoolType>::Ptr, PoolType> ReqList;
    BlockDeviceInterface& BlockDev;
    Atomic ReqCompleteCount;
    Event ReqCompleteEvent;
    Error Result;
};

typedef Core::BioList<Core::Memory::PoolType::NoIO> NoIOBioList;

}