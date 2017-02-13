#include "page.h"
#include "kapi.h"

namespace Core
{

Page::Page(Memory::PoolType poolType, Error& err)
    : PoolType(poolType)
    , PagePtr(nullptr)
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

void* Page::Map()
{
    return get_kapi()->map_page(PagePtr);
}

void Page::Unmap()
{
    get_kapi()->unmap_page(PagePtr);
}

void* Page::MapAtomic()
{
    return get_kapi()->map_page_atomic(PagePtr);
}

void Page::UnmapAtomic(void* va)
{
    get_kapi()->unmap_page_atomic(va);
}

void* Page::GetPage()
{
    return PagePtr;
}

int Page::GetPageSize()
{
    return get_kapi()->get_page_size();
}

void Page::Zero()
{
    void* va = MapAtomic();
    Memory::MemSet(va, 0, GetPageSize());
    UnmapAtomic(va);
}

Error Page::FillRandom(Random& rng)
{
    void* va = MapAtomic();
    Error err = rng.GetBytes(va, GetPageSize());
    UnmapAtomic(va);
    return err;
}

int Page::CompareContent(Page& other)
{
    void* va = MapAtomic();
    void* vaOther = other.MapAtomic();
    int rc = Memory::MemCmp(va, vaOther, GetPageSize());
    other.UnmapAtomic(vaOther);
    UnmapAtomic(va);
    return rc;
}

void Page::Read(void *buf, size_t len)
{
    void* va = MapAtomic();
    Core::Memory::MemCpy(buf, va, Core::Memory::Min<size_t>(len, GetPageSize()));
    UnmapAtomic(va);
}

void Page::Write(const void *buf, size_t len)
{
    void* va = MapAtomic();
    Core::Memory::MemCpy(va, buf, Core::Memory::Min<size_t>(len, GetPageSize()));
    UnmapAtomic(va);
}

Page::~Page()
{
    if (PagePtr != nullptr)
    {
        get_kapi()->free_page(PagePtr);
        PagePtr = nullptr;
    }
}


PageMap::PageMap(Page& page)
    : PageRef(page)
    , Address(nullptr)
{
    Address = PageRef.Map();
}

PageMap::~PageMap()
{
    Unmap();
}

void* PageMap::GetAddress()
{
    return Address;
}

void PageMap::Unmap()
{
    if (Address != nullptr)
    {
        PageRef.Unmap();
        Address = nullptr;
    }
}

}