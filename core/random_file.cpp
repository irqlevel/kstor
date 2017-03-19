#include "random_file.h"
#include "astring.h"

namespace Core
{

RandomFile::RandomFile(Error& err, bool pseudoRandom)
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

    err = File.Open(devName, true, false);
    if (!err.Ok())
    {
        trace(0, "Can't open dev random file %s, err %d", devName.GetConstBuf(), err.GetCode());
        return;
    }

    trace(3, "Random 0x%p dev %s ctor", this, devName.GetConstBuf());
}

Error RandomFile::GetBytes(void* buf, unsigned long len)
{
    Error err = File.Read(0, buf, len);
    if (!err.Ok())
    {
        trace(0, "Can't read dev random file, err %d", err.GetCode());
    }
    return err;
}

unsigned long RandomFile::GetUlong()
{
    unsigned long result;

    Error err = GetBytes(&result, sizeof(result));
    if (!err.Ok())
    {
        return static_cast<unsigned long>(-1);
    }

    return result;
}

RandomFile::~RandomFile()
{
    trace(3, "Random 0x%p dtor", this);
}

}