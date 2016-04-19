#include "spinlock.h"

SpinLock::SpinLock()
{
}

SpinLock::SpinLock(int& err, MemType memType)
{
    if (err)
        return;

    Lock = get_kapi()->spinlock_create(get_kapi_mem_flag(memType));
    if (!Lock)
    {
        err = E_NO_MEM;
        return;
    }
    err = E_OK;
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
