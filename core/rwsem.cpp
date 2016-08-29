#include "rwsem.h"

RWSem::RWSem()
{
}

RWSem::RWSem(Error& err, MemType memType)
{
    if (err != Error::Success)
        return;

    Lock = get_kapi()->rwsem_create(get_kapi_mem_flag(memType));
    if (!Lock)
    {
        err = Error::NoMemory;
        return;
    }
    err = Error::Success;
}

void RWSem::Acquire()
{
    get_kapi()->rwsem_down_write(Lock);
}

void RWSem::Release()
{
    get_kapi()->rwsem_up_write(Lock);
}

void RWSem::AcquireShared()
{
    get_kapi()->rwsem_down_read(Lock);
}

void RWSem::ReleaseShared()
{
    get_kapi()->rwsem_up_read(Lock);
}

RWSem::~RWSem()
{
    if (Lock)
        get_kapi()->rwsem_delete(Lock);
}

RWSem::RWSem(RWSem&& other)
{
    Lock = other.Lock;
    other.Lock = nullptr;
}

RWSem& RWSem::operator=(RWSem&& other)
{
    if (Lock)
        get_kapi()->rwsem_delete(Lock);

    Lock = other.Lock;
    other.Lock = nullptr;
    return *this;
}
