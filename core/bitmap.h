#pragma once

#include "error.h"
#include "memory.h"

class Bitmap
{
public:
    Bitmap(long bitCount, Memory::PoolType poolType, Error& err);
    void SetBit(long bitNumber);
    void ClearBit(long bitNumber);
    bool TestAndSetBit(long bitNumber);
    bool TestAndClearBit(long bitNumber);
    virtual ~Bitmap();

private:
    unsigned long* GetBits();

private:

    char *Bits;
    long BitCount;
};