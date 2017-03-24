#include "bitmap.h"
#include "memory.h"
#include "bitops.h"
#include "new.h"

namespace Core
{

Bitmap::Bitmap(void *buf, size_t size)
    : Buf(buf)
    , Size(size)
{
    BitSize = 8 * size;
}

Bitmap::~Bitmap()
{
}

Error Bitmap::SetBit(size_t bit)
{
    if (bit >= BitSize)
        return MakeError(Error::Overflow);

    BitOps::SetBit(bit, static_cast<unsigned long *>(Buf));
    return MakeError(Error::Success);
}

Error Bitmap::ClearBit(size_t bit)
{
    if (bit >= BitSize)
        return MakeError(Error::Overflow);

    BitOps::ClearBit(bit, static_cast<unsigned long *>(Buf));
    return MakeError(Error::Success);
}

Error Bitmap::TestAndSetBit(size_t bit, bool& oldValue)
{
    if (bit >= BitSize)
        return MakeError(Error::Overflow);

    oldValue = BitOps::TestAndSetBit(bit, static_cast<unsigned long *>(Buf));
    return MakeError(Error::Success);
}

Error Bitmap::TestAndClearBit(size_t bit, bool& oldValue)
{
    if (bit >= BitSize)
        return MakeError(Error::Overflow);

    oldValue = BitOps::TestAndClearBit(bit, static_cast<unsigned long *>(Buf));
    return MakeError(Error::Success);
}

Error Bitmap::FindSetZeroBit(unsigned long* value, size_t maxBits, size_t& bit)
{
    if (maxBits > Memory::SizeOfInBits<unsigned long>())
        return MakeError(Error::Overflow);

    if (*value == ~(static_cast<unsigned long>(0)))
        return MakeError(Error::NotFound);

    for (size_t i = 0; i < maxBits; i++)
    {
        if (BitOps::TestAndSetBit(i, value) == false)
        {
            bit = i;
            return MakeError(Error::Success);
        }
    }

    return MakeError(Error::NotFound);
}

Error Bitmap::FindSetZeroBit(size_t& bit)
{
    size_t i, foundBit, restBits = BitSize % Memory::SizeOfInBits<unsigned long>();
    unsigned long *ulongPtr = static_cast<unsigned long *>(Buf);

    for (i = 0; i < BitSize / Memory::SizeOfInBits<unsigned long>(); i++, ulongPtr++)
    {
        auto err = FindSetZeroBit(ulongPtr, Memory::SizeOfInBits<unsigned long>(), foundBit);
        if (err == Error::NotFound)
        {
            continue;
        }

        if (!err.Ok())
            return err;

        bit = i * Memory::SizeOfInBits<unsigned long>() + foundBit;
        return MakeError(Error::Success);
    }

    auto err = FindSetZeroBit(ulongPtr, restBits, foundBit);
    if (!err.Ok())
        return err;

    bit = i * Memory::SizeOfInBits<unsigned long>() + foundBit;
    return MakeError(Error::Success);
}

void* Bitmap::GetBuf()
{
    return Buf;
}

}