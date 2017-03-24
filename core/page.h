#pragma once

#include "memory.h"
#include "error.h"
#include "random.h"
#include "random_file.h"
#include "shared_ptr.h"
#include "hex.h"
#include "bitmap.h"

namespace Core
{

class PageInterface : public BitmapInterface
{
public:
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
    virtual void* MapAtomic() = 0;
    virtual void UnmapAtomic(void* va) = 0;
    virtual size_t GetSize() const = 0;
    virtual size_t Read(void *buf, size_t len, size_t off) const = 0;
    virtual size_t Write(const void *buf, size_t len, size_t off) = 0;
    virtual void Zero() = 0;
    virtual AString ToHex(size_t len) const = 0;
};

template<Memory::PoolType PoolType = Memory::PoolType::Kernel>
class Page : public PageInterface
{
public:
    using Ptr = SharedPtr<Page<PoolType>, PoolType>;

public:
    Page(Error& err)
        : PagePtr(nullptr)
    {
        if (!err.Ok())
        {
            return;
        }

        PagePtr = get_kapi()->alloc_page(get_kapi_pool_type(PoolType));
        if (PagePtr == nullptr)
        {
            err = MakeError(Error::NoMemory);
            return;
        }
    }

    virtual void* Map() override
    {
        return get_kapi()->map_page(PagePtr);
    }

    virtual void Unmap() override
    {
        get_kapi()->unmap_page(PagePtr);
    }

    virtual void* MapAtomic() override
    {
        return get_kapi()->map_page_atomic(PagePtr);
    }

    virtual void UnmapAtomic(void* va) override
    {
        get_kapi()->unmap_page_atomic(va);
    }

    void* GetPagePtr()
    {
        return PagePtr;
    }

    virtual size_t GetSize() const override
    {
        return GetPageSize();
    }

    static size_t GetPageSize()
    {
        return get_kapi()->get_page_size();
    }

    virtual void Zero() override
    {
        void* va = MapAtomic();
        Memory::MemSet(va, 0, GetSize());
        UnmapAtomic(va);
    }

    Error FillRandom(RandomFile& rng)
    {
        void* va = MapAtomic();
        Error err = rng.GetBytes(va, GetSize());
        UnmapAtomic(va);
        return err;
    }

    void FillRandom()
    {
        void* va = MapAtomic();
        Random::GetBytes(va, GetSize());
        UnmapAtomic(va);
    }

    int CompareContent(PageInterface& other)
    {
        void* va = MapAtomic();
        void* vaOther = other.MapAtomic();
        int rc = Memory::MemCmp(va, vaOther, GetSize());
        other.UnmapAtomic(vaOther);
        UnmapAtomic(va);
        return rc;
    }

    virtual ~Page()
    {
        if (PagePtr != nullptr)
        {
            get_kapi()->free_page(PagePtr);
            PagePtr = nullptr;
        }
    }

    virtual size_t Read(void *buf, size_t len, size_t off) const override
    {
        if (off >= GetSize())
            return 0;

        void* va = ConstMapAtomic();
        size_t size = Memory::Min<size_t>(len, GetSize() - off);
        Memory::MemCpy(buf, Memory::MemAdd(va, off), size);
        ConstUnmapAtomic(va);
        return size;
    }

    virtual size_t Write(const void *buf, size_t len, size_t off) override
    {
        if (off >= GetSize())
            return 0;

        void* va = MapAtomic();
        size_t size = Memory::Min<size_t>(len, GetSize() - off);
        Memory::MemCpy(Memory::MemAdd(va, off), buf, size);
        UnmapAtomic(va);
        return size;
    }

    static SharedPtr<Page<PoolType>, PoolType> Create(Error& err)
    {
        SharedPtr<Page<PoolType>, PoolType> page = MakeShared<Page<PoolType>, PoolType>(err);
        if (page.Get() == nullptr)
        {
            err = MakeError(Error::NoMemory);
            return page;
        }

        if (!err.Ok())
            page.Reset();

        return page;
    }

    virtual AString ToHex(size_t len) const override
    {
        AString result;

        auto size = GetSize();
        void* va = ConstMapAtomic();
        if (len > size)
            len = size;
        result = Hex::Encode(static_cast<const unsigned char *>(va), len);
        ConstUnmapAtomic(va);
        return result;
    }

    virtual Error SetBit(size_t bit) override
    {
        Bitmap bitmap(MapAtomic(), GetSize());
        auto err = bitmap.SetBit(bit);
        UnmapAtomic(bitmap.GetBuf());
        return err;
    }

    virtual Error ClearBit(size_t bit) override
    {
        Bitmap bitmap(MapAtomic(), GetSize());
        auto err = bitmap.ClearBit(bit);
        UnmapAtomic(bitmap.GetBuf());
        return err;
    }

    virtual Error TestAndSetBit(size_t bit, bool& oldValue) override
    {
        Bitmap bitmap(MapAtomic(), GetSize());
        auto err = bitmap.TestAndSetBit(bit, oldValue);
        UnmapAtomic(bitmap.GetBuf());
        return err;
    }

    virtual Error TestAndClearBit(size_t bit, bool& oldValue) override
    {
        Bitmap bitmap(MapAtomic(), GetSize());
        auto err = bitmap.TestAndClearBit(bit, oldValue);
        UnmapAtomic(bitmap.GetBuf());
        return err;
    }

    virtual Error FindSetZeroBit(size_t& bit) override
    {
        Bitmap bitmap(MapAtomic(), GetSize());
        auto err = bitmap.FindSetZeroBit(bit);
        UnmapAtomic(bitmap.GetBuf());
        return err;
    }

private:
    Page(const Page& other) = delete;
    Page(Page&& other) = delete;
    Page& operator=(const Page& other) = delete;
    Page& operator=(Page&& other) = delete;

    void* ConstMapAtomic() const
    {
        return get_kapi()->map_page_atomic(PagePtr);
    }

    void ConstUnmapAtomic(void* va) const
    {
        get_kapi()->unmap_page_atomic(va);
    }

    void* PagePtr;
};

class PageMap
{
public:
    PageMap(PageInterface& page);
    virtual ~PageMap();
    void* GetAddress();
    void Unmap();
private:
    PageMap(PageMap&& other) = delete;
    PageMap(const PageMap& other) = delete;
    PageMap& operator=(const PageMap& other) = delete;
    PageMap& operator=(PageMap&& other) = delete;

    PageInterface& PageRef;
    void* Address;
};

}