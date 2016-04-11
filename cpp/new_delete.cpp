#include "main.h"
#include "new_delete.h"

void *newFunc(size_t size)
{
	return get_kapi()->malloc(size);
}

void deleteFunc(void *ptr)
{
	get_kapi()->free(ptr);
}

void* operator new(size_t size)
{
    return newFunc(size);
}

void* operator new[](size_t size)
{
    return newFunc(size);
}

void operator delete(void* ptr)
{
	deleteFunc(ptr);
}

void operator delete[](void* ptr)
{
	deleteFunc(ptr);
}
