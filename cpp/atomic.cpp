#include "atomic.h"
#include "error.h"

Atomic::Atomic()
{
}

Atomic::Atomic(int value, int& err, MemType memType)
{
    if (err)
        return;

    pAtomic = get_kapi()->atomic_create(value, get_kapi_mem_flag(memType));
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

Atomic::Atomic(Atomic&& other)
{
    pAtomic = other.pAtomic;
    other.pAtomic = nullptr;
}

Atomic& Atomic::operator=(Atomic&& other)
{
    if (pAtomic)
    {
        get_kapi()->atomic_delete(pAtomic);
        pAtomic = nullptr;
    }
    pAtomic = other.pAtomic;
    other.pAtomic = nullptr;
    return *this;
}
