#include "atomic.h"
#include "error.h"

Atomic::Atomic(int value, int& err)
{
    if (err)
        return;

    pAtomic = get_kapi()->atomic_create(value);
    if (!pAtomic)
    {
        err = E_NO_MEM;
        return;
    }
}

void Atomic::Inc()
{
    get_kapi()->atomic_inc(pAtomic);
}

bool Atomic::DecAndTest()
{
    return get_kapi()->atomic_dec_and_test(pAtomic);
}

int Atomic::Get()
{
    return get_kapi()->atomic_read(pAtomic);
}

Atomic::~Atomic()
{
    if (pAtomic)
        get_kapi()->atomic_delete(pAtomic);
}
