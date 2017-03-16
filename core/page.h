#pragma once

#include "memory.h"
#include "error.h"
#include "random.h"
#include "shared_ptr.h"
#include "hex.h"

namespace Core
{

class PageInterface
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

template<Memory::PoolType PoolType>
class Page : public PageInterface
{
public:
    typedef SharedPtr<Page<PoolType>, PoolType> Ptr;

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
            err.SetNoMemory();
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
        size_t size = Core::Memory::Min<size_t>(len, GetSize() - off);
        Core::Memory::MemCpy(buf, Core::Memory::MemAdd(va, off), size);
        ConstUnmapAtomic(va);
        return size;
    }

    virtual size_t Write(const void *buf, size_t len, size_t off) override
    {
        if (off >= GetSize())
            return 0;

        void* va = MapAtomic();
        size_t size = Core::Memory::Min<size_t>(len, GetSize() - off);
        Core::Memory::MemCpy(Core::Memory::MemAdd(va, off), buf, size);
        UnmapAtomic(va);
        return size;
    }

    static SharedPtr<Page<PoolType>, PoolType> Create(Error& err)
    {
        SharedPtr<Page<PoolType>, PoolType> page = MakeShared<Page<PoolType>, PoolType>(err);
        if (page.Get() == nullptr)
        {
            err = Error::NoMemory;
            return page;
        }

        if (!err.Ok())
            page.Reset();

        return page;
    }

    virtual Core::AString ToHex(size_t len) const override
    {
        Core::AString result;

        auto size = GetSize();
        void* va = ConstMapAtomic();
        if (len > size)
            len = size;
        result = Hex::Encode(static_cast<const unsigned char *>(va), len);
        ConstUnmapAtomic(va);
        return result;
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

using PagePtr = SharedPtr<Page<Memory::PoolType::Kernel>, Memory::PoolType::Kernel>;

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