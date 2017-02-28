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

private:
    Bio(const Bio& other) = delete;
    Bio(Bio&& other) = delete;
    Bio& operator=(const Bio& other) = delete;
    Bio& operator=(Bio&& other) = delete;

    void EndIo(int err)
    {
        trace(4, "Bio 0x%p bio 0x%p endio err %d", this, BioPtr, err);
        IoError.SetCode(err);
        EndIoEvent.Set();
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
};

/*
class BioList
{
public:
    BioList(BlockDevice& blockDevice);
    virtual ~BioList();

    Error AddIo(Page& page, unsigned long long position, bool write);

    void Submit(bool flushFua = false);
    void Wait();
    Error GetResult();
private:
    BioList(const BioList& other) = delete;
    BioList(BioList&& other) = delete;
    BioList& operator=(const BioList& other) = delete;
    BioList& operator=(BioList&& other) = delete;

    LinkedList<BioPtr, Memory::PoolType::Kernel> ReqList;
    BlockDevice& BlockDev;
    Atomic ReqCompleteCount;
};
*/

}