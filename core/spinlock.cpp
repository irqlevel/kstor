#include "spinlock.h"

SpinLock::SpinLock()
{
    get_kapi()->spinlock_init(&Lock);
}

void SpinLock::Acquire()
{
    get_kapi()->spinlock_lock(&Lock);
}

void SpinLock::Release()
{
    get_kapi()->spinlock_unlock(&Lock);
}

SpinLock::~SpinLock()
{
}
