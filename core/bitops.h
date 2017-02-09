#pragma once

namespace Core
{

class BitOps
{
public:
    static void SetBit(long nr, unsigned long *addr);
    static void ClearBit(long nr, unsigned long *addr);
    static bool TestAndSetBit(long nr, unsigned long *addr);
    static bool TestAndClearBit(long nr, unsigned long *addr);
    static unsigned int Le32ToCpu(unsigned int value);
    static unsigned int CpuToLe32(unsigned int value);
    static unsigned long long Le64ToCpu(unsigned long long value);
    static unsigned long long CpuToLe64(unsigned long long value);
};

}