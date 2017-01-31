#include "bitmap.h"
#include "memory.h"
#include "bitops.h"
#include "new.h"

Bitmap::Bitmap(long bitCount, Memory::PoolType poolType, Error& err)
    : Bits(nullptr)
    , BitCount(0)
{
    if (bitCount <= 0)
    {
        err = Error::InvalidValue;
        return;
    }

    if (bitCount % 8)
    {
        err = Error::InvalidValue;
        return;
    }

    BitCount = bitCount;
    Bits = new (poolType) char[BitCount / 8];
}

unsigned long* Bitmap::GetBits()
{
    return reinterpret_cast<unsigned long *>(Bits);
}

void Bitmap::SetBit(long bitNumber)
{
    if (bitNumber >= 0 && bitNumber < BitCount)
    {
        BitOps::SetBit(bitNumber, GetBits());
    }
}

void Bitmap::ClearBit(long bitNumber)
{
    if (bitNumber >= 0 && bitNumber < BitCount)
    {
        BitOps::ClearBit(bitNumber, GetBits());
    }    
}

bool Bitmap::TestAndSetBit(long bitNumber)
{
    if (bitNumber >= 0 && bitNumber < BitCount)
    {
        return BitOps::TestAndClearBit(bitNumber, GetBits());
    }

    return false;
}

bool Bitmap::TestAndClearBit(long bitNumber)
{
    if (bitNumber >= 0 && bitNumber < BitCount)
    {
        return BitOps::TestAndClearBit(bitNumber, GetBits());
    }

    return false;
}

Bitmap::~Bitmap()
{
    if (Bits != nullptr)
    {
        delete [] Bits;
    }
}
