#include "page.h"
#include "kapi.h"

Page::Page(Memory::PoolType poolType, Error& err)
    : PoolType(poolType)
    , PagePtr(nullptr)
{
    if (err != Error::Success)
    {
        return;
    }
    PagePtr = get_kapi()->alloc_page(get_kapi_pool_type(PoolType));
    if (PagePtr == nullptr)
    {
        err = Error::NoMemory;
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

Page::~Page()
{
    if (PagePtr != nullptr)
    {
        get_kapi()->free_page(PagePtr);
        PagePtr = nullptr;
    }
}

