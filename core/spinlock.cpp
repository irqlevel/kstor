#include "spinlock.h"

SpinLock::SpinLock()
{
}

SpinLock::SpinLock(Error& err, Memory::PoolType poolType)
{
    if (err != Error::Success)
        return;

    Lock = get_kapi()->spinlock_create(get_kapi_gfp_flags(poolType));
    if (!Lock)
    {
        err = Error::NoMemory;
        return;
    }
    err = Error::Success;
}

void SpinLock::Acquire()
{
    get_kapi()->spinlock_lock(Lock);
}

void SpinLock::Release()
{
    get_kapi()->spinlock_unlock(Lock);
}

SpinLock::~SpinLock()
{
    if (Lock)
        get_kapi()->spinlock_delete(Lock);
}

SpinLock::SpinLock(SpinLock&& other)
{
    Lock = other.Lock;
    other.Lock = nullptr;
}

SpinLock& SpinLock::operator=(SpinLock&& other)
{
    if (Lock)
        get_kapi()->spinlock_delete(Lock);

    Lock = other.Lock;
    other.Lock = nullptr;
    return *this;
}
