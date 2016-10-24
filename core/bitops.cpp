#include "bitops.h"
#include "kapi.h"

void BitOps::SetBit(long nr, unsigned long *addr)
{
    get_kapi()->set_bit(nr, addr);
}

void BitOps::ClearBit(long nr, unsigned long *addr)
{
    get_kapi()->clear_bit(nr, addr);
}

bool BitOps::TestAndSetBit(long nr, unsigned long *addr)
{
    return (get_kapi()->test_and_set_bit(nr, addr) != 0) ? true : false;
}

bool BitOps::TestAndClearBit(long nr, unsigned long *addr)
{
    return (get_kapi()->test_and_clear_bit(nr, addr) != 0) ? true : false;
}