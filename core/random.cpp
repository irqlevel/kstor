#include "random.h"
#include "memory.h"
#include "kapi.h"

namespace Core
{

void Random::GetBytes(void* buf, int len)
{
    get_kapi()->get_random_bytes(buf, len);
}

uint64_t Random::GetUint64()
{
    uint64_t result;

    Random::GetBytes(&result, sizeof(result));
    return result;
}

byte_t Random::GetByte()
{
    byte_t result;

    Random::GetBytes(&result, sizeof(result));
    return result;
}

size_t Random::Log2(size_t value)
{
    size_t result = 0;

    while (value > 0)
    {
        result++;
        value >>= 1;
    }
    return result;
}

size_t Random::GetSizeT()
{
    size_t result;

    Random::GetBytes(&result, sizeof(result));
    return result;
}

size_t Random::GetSizeT(size_t upper)
{
    if (upper < 2)
        return 0;

    if ((upper & (upper - 1)) == 0)
        return GetSizeT() & (upper -1);

    size_t value, log;
    log = Random::Log2(upper);
    for (;;)
    {
        value = GetSizeT() & ((static_cast<size_t>(1) << log) - 1);
        if (value < upper)
            break;
    }

    return value;
}

}