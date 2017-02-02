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
};

}