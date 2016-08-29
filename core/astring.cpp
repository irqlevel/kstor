#include "astring.h"

#include <stdio.h>
#include <string.h>

AString::AString()
    : Buf(Memory::PoolType::Kernel)
{
}

AString::AString(const char* s, Memory::PoolType poolType, Error& err)
    : Buf(poolType)
{
    if (err != Error::Success)
        return;

    size_t len = strlen(s);
    size_t size = len + 1;

    if (!Buf.Reserve(size))
    {
        err = Error::NoMemory;
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        Buf.PushBack(s[i]);
    }
    err = Error::Success;
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

AString::AString(const AString& other, Error& err)
    : Buf(other.Buf, err)
{
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

int AString::Compare(const AString& key1, const AString &key2)
{
    return key1.Compare(key2);
}

size_t AString::Hash() const
{
    return 0;
}

size_t AString::Hash(const AString& key)
{
    return key.Hash();
}
