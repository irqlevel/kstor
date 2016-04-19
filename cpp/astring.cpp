#include "astring.h"

#include <stdio.h>
#include <string.h>

AString::AString(const char* s, MemType memType, int& err)
    : Buf(memType)
{
    if (err)
        return;

    size_t len = strlen(s);
    size_t size = len + 1;

    if (!Buf.Reserve(size))
    {
        err = E_NO_MEM;
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        Buf.PushBack(s[i]);
    }
    err = E_OK;
}

const char* AString::GetBuf() const
{
    return Buf.GetBuf();
}

size_t AString::GetLen() const
{
    if (!Buf.GetSize())
        return 0;

    return Buf.GetSize() - 1;
}

AString::AString(AString&& other)
    : Buf(util::move(other.Buf))
{
}

AString& AString::operator=(AString&& other)
{
    Buf.Clear();
    Buf = util::move(other.Buf);
    return *this;
}

AString::~AString()
{
}

int AString::Compare(const AString& other) const
{
    size_t len = GetLen();
    size_t otherLen = other.GetLen();

    if (len > otherLen)
    {
        return 1;
    }

    if (len < otherLen)
    {
        return -1;
    }

    const char* buf = GetBuf();
    const char* otherBuf = other.GetBuf();

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] > otherBuf[i])
            return 1;
        if (buf[i] < otherBuf[i])
            return -1;
    }

    return 0;
}
