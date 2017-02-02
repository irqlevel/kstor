#include "spinlock.h"

namespace Core
{

SpinLock::SpinLock()
    : IrqFlags(0)
{
    get_kapi()->spinlock_init(&Lock);
}

void SpinLock::Acquire()
{
    get_kapi()->spinlock_lock_irqsave(&Lock, &IrqFlags);
}

void SpinLock::Release()
{
    get_kapi()->spinlock_unlock_irqrestore(&Lock, IrqFlags);
}

SpinLock::~SpinLock()
{
}

}
