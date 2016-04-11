#include "kobj.h"

KObj::KObj(int value)
    : Value(value)
{
    PRINTF("ctor %p value %d\n", this, Value);
}

int KObj::GetValue()
{
    return Value;
}

KObj::~KObj()
{
    PRINTF("dtor %p value %d\n", this, Value);
}
