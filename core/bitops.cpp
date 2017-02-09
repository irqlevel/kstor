#include "bitops.h"
#include "kapi.h"

namespace Core
{

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

unsigned int BitOps::Le32ToCpu(unsigned int value)
{
    return get_kapi()->le32_to_cpu(value);
}

unsigned int BitOps::CpuToLe32(unsigned int value)
{
    return get_kapi()->cpu_to_le32(value);
}

unsigned long long BitOps::Le64ToCpu(unsigned long long value)
{
    return get_kapi()->le64_to_cpu(value);
}

unsigned long long BitOps::CpuToLe64(unsigned long long value)
{
    return get_kapi()->cpu_to_le64(value);
}

}