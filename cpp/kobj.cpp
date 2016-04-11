#include "kobj.h"

KObj::KObj(int value)
	: Value(value)
{
	PRINTF("ctor %p value %d\n", this, Value);
}

KObj::~KObj()
{
	PRINTF("dtor %p value %d\n", this, Value);
}
