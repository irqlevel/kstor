#include "kapi.h"
#include "new.h"

void *newFunc(size_t size, Core::Memory::PoolType poolType)
{
    return get_kapi()->kmalloc(size, get_kapi_pool_type(poolType));
}

void deleteFunc(void *ptr)
{
    get_kapi()->kfree(ptr);
}

void* operator new(size_t size)
{
    return newFunc(size, Core::Memory::PoolType::Kernel);
}

void* operator new[](size_t size)
{
    return newFunc(size, Core::Memory::PoolType::Kernel);
}

void* operator new(size_t size, Core::Memory::PoolType poolType)
{
    return newFunc(size, poolType);
}

void* operator new[](size_t size, Core::Memory::PoolType poolType)
{
    return newFunc(size, poolType);
}

void operator delete(void* ptr)
{
    deleteFunc(ptr);
}

void operator delete[](void* ptr)
{
    deleteFunc(ptr);
}
