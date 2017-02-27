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

void* Page::GetPagePtr()
{
    return PagePtr;
}

size_t Page::GetSize()
{
    return get_kapi()->get_page_size();
}

void Page::Zero()
{
    void* va = MapAtomic();
    Memory::MemSet(va, 0, GetSize());
    UnmapAtomic(va);
}

Error Page::FillRandom(Random& rng)
{
    void* va = MapAtomic();
    Error err = rng.GetBytes(va, GetSize());
    UnmapAtomic(va);
    return err;
}

int Page::CompareContent(Page& other)
{
    void* va = MapAtomic();
    void* vaOther = other.MapAtomic();
    int rc = Memory::MemCmp(va, vaOther, GetSize());
    other.UnmapAtomic(vaOther);
    UnmapAtomic(va);
    return rc;
}

size_t Page::Read(void *buf, size_t len, size_t off)
{
    if (off >= GetSize())
        return 0;

    void* va = MapAtomic();
    size_t size = Core::Memory::Min<size_t>(len, GetSize() - off);
    Core::Memory::MemCpy(buf, Core::Memory::MemAdd(va, off), size);
    UnmapAtomic(va);
    return size;
}

size_t Page::Write(const void *buf, size_t len, size_t off)
{
    if (off >= GetSize())
        return 0;

    void* va = MapAtomic();
    size_t size = Core::Memory::Min<size_t>(len, GetSize() - off);
    Core::Memory::MemCpy(Core::Memory::MemAdd(va, off), buf, size);
    UnmapAtomic(va);
    return size;
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