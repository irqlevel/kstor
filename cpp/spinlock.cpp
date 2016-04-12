#include "spinlock.h"

SpinLock::SpinLock(int& err)
{
    if (err)
        return;

    Lock = get_kapi()->spinlock_create();
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
    get_kapi()->spinlock_delete(Lock);
}
