#include "atomic.h"
#include "error.h"

namespace Core
{

Atomic::Atomic()
{
    Set(0);
}

Atomic::Atomic(int value)
{
    Set(value);
}

void Atomic::Inc()
{
    get_kapi()->atomic_inc(&Value);
}

bool Atomic::DecAndTest()
{
    return get_kapi()->atomic_dec_and_test(&Value);
}

void Atomic::Set(int value)
{
    get_kapi()->atomic_set(&Value, value);
}

int Atomic::Get()
{
    return get_kapi()->atomic_read(&Value);
}

Atomic::~Atomic()
{
}

Atomic::Atomic(Atomic&& other)
{
    Set(other.Get());
}

Atomic& Atomic::operator=(Atomic&& other)
{
    Set(other.Get());
    return *this;
}

}