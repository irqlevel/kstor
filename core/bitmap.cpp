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
        return Error::Overflow;

    BitOps::SetBit(bit, static_cast<unsigned long *>(Buf));
    return Error::Success;
}

Error Bitmap::ClearBit(size_t bit)
{
    if (bit >= BitSize)
        return Error::Overflow;

    BitOps::ClearBit(bit, static_cast<unsigned long *>(Buf));
    return Error::Success;
}

Error Bitmap::TestAndSetBit(size_t bit, bool& oldValue)
{
    if (bit >= BitSize)
        return Error::Overflow;

    oldValue = BitOps::TestAndSetBit(bit, static_cast<unsigned long *>(Buf));
    return Error::Success;
}

Error Bitmap::TestAndClearBit(size_t bit, bool& oldValue)
{
    if (bit >= BitSize)
        return Error::Overflow;

    oldValue = BitOps::TestAndClearBit(bit, static_cast<unsigned long *>(Buf));
    return Error::Success;
}

Error Bitmap::FindZeroBit(size_t& bit)
{
    return Error::NotImplemented;
}

void* Bitmap::GetBuf()
{
    return Buf;
}

}