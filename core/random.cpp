#include "random.h"
#include "astring.h"
#include "memory.h"
#include "trace.h"

namespace Core
{

Random::Random(Error& err, bool pseudoRandom)
{
    if (!err.Ok())
    {
        return;
    }

    AString devName((pseudoRandom) ? "/dev/urandom" : "/dev/random", err);
    if (!err.Ok())
    {
        trace(0, "Can't allocate string");
        return;
    }

    err = DevRandomFile.Open(devName, true, false);
    if (!err.Ok())
    {
        trace(0, "Can't open dev random file %s, err %d", devName.GetBuf(), err.GetCode());
        return;
    }

    trace(3, "Random 0x%p dev %s ctor", this, devName.GetBuf());
}

Error Random::GetBytes(void* buf, unsigned long len)
{
    Error err = DevRandomFile.Read(0, buf, len);
    if (!err.Ok())
    {
        trace(0, "Can't read dev random file, err %d", err.GetCode());
    }
    return err;
}

unsigned long Random::GetUlong()
{
    unsigned long result;

    Error err = GetBytes(&result, sizeof(result));
    if (!err.Ok())
    {
        return static_cast<unsigned long>(-1);
    }

    return result;
}

Random::~Random()
{
    trace(3, "Random 0x%p dtor", this);
}

}