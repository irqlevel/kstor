#include "main.h"
#include "new_delete.h"

void *newFunc(size_t size, MemType memType)
{
    return get_kapi()->kmalloc(size, get_kapi_mem_flag(memType));
}

void deleteFunc(void *ptr)
{
    get_kapi()->kfree(ptr);
}

void* operator new(size_t size)
{
    return newFunc(size, MemType::Kernel);
}

void* operator new[](size_t size)
{
    return newFunc(size, MemType::Kernel);
}

void* operator new(size_t size, MemType memType)
{
    return newFunc(size, memType);
}

void* operator new[](size_t size, MemType memType)
{
    return newFunc(size, memType);
}

void operator delete(void* ptr)
{
    deleteFunc(ptr);
}

void operator delete[](void* ptr)
{
    deleteFunc(ptr);
}
