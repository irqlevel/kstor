#include "main.h"
#include "new_delete.h"

void *newFunc(size_t size, Memory::PoolType poolType)
{
    return get_kapi()->kmalloc(size, get_kapi_gfp_flags(poolType));
}

void deleteFunc(void *ptr)
{
    get_kapi()->kfree(ptr);
}

void* operator new(size_t size)
{
    return newFunc(size, Memory::PoolType::Kernel);
}

void* operator new[](size_t size)
{
    return newFunc(size, Memory::PoolType::Kernel);
}

void* operator new(size_t size, Memory::PoolType poolType)
{
    return newFunc(size, poolType);
}

void* operator new[](size_t size, Memory::PoolType poolType)
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
