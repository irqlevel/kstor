#pragma once

#include "vector.h"
#include "shared_ptr.h"
#include "error.h"
#include "memory.h"

namespace Core
{

template <Memory::PoolType PoolType = Memory::PoolType::Kernel>
class AStringBase
{
public:
    AStringBase()
    {        
    }

    AStringBase(const char* s, size_t len, Error& err)
    {
        if (!err.Ok())
        {
            return;
        }

        if (!Buf.Reserve(len + 1))
        {
            err = MakeError(Error::NoMemory);
            return;
        }

        for (size_t i = 0; i < len; i++)
        {
            char c = s[i];
            if (c == '\0')
                break;

            if (!Buf.PushBack(c))
            {
                err = MakeError(Error::NoMemory);
                Buf.Clear();
                return;
            }
        }

        if (!Buf.PushBack('\0'))
        {
            err = MakeError(Error::NoMemory);
            Buf.Clear();
            return;
        }

        err = MakeError(Error::Success);
    }

    AStringBase(const char* s, Error& err)
        : AStringBase(s, Memory::StrLen(s), err)
    {
    }

    AStringBase(const AStringBase& s, Error& err)
        : AStringBase(s.GetConstBuf(), err)
    {
    }

    virtual ~AStringBase()
    {
    }

    const char* GetConstBuf() const
    {
        return Buf.GetConstBuf();
    }

    char* GetBuf()
    {
        return Buf.GetBuf();
    }

    bool ReserveAndUse(size_t len)
    {
        if (!Buf.ReserveAndUse(len + 1))
            return false;
        return true;
    }

    bool Truncate(size_t len)
    {
        if (!Buf.Truncate(len + 1))
            return false;
        return true;
    }

    size_t GetLen() const
    {
        size_t size = Buf.GetSize();
        if (size == 0)
        {
            return 0;
        }

        return size - 1;
    }

    AStringBase(AStringBase&& other)
        : Buf(Memory::Move(other.Buf))
    {
    }

    AStringBase& operator=(AStringBase&& other)
    {
        Buf.Clear();
        Buf = Memory::Move(other.Buf);
        return *this;
    }

    int Compare(const AStringBase& other) const 
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

        const char* buf = GetConstBuf();
        const char* otherBuf = other.GetConstBuf();

        for (size_t i = 0; i < len; i++)
        {
            if (buf[i] > otherBuf[i])
            {
                return 1;
            }

            if (buf[i] < otherBuf[i])
            {
                return -1;
            }
        }

        return 0;
    }

    size_t Hash() const 
    {
        size_t hash = 5381;
        size_t c;

        const char* buf = GetConstBuf();
        for (size_t i = 0; i < GetLen(); i++)
        {
            c = buf[i];
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }

        return hash;
    }

    static int Compare(const AStringBase& key1, const AStringBase& key2)
    {
        return key1.Compare(key2);
    }

    static size_t Hash(const AStringBase& key)
    {
        return key.Hash();
    }

    Error Append(const AStringBase& other)
    {
        size_t otherLen = other.GetLen();
        size_t oldLen = GetLen();
        size_t newLen = oldLen + otherLen;

        if (!ReserveAndUse(newLen))
            return MakeError(Error::NoMemory);

        char* dst = Buf.GetBuf();
        const char* src = other.Buf.GetConstBuf();
        for (size_t i = oldLen, j = 0; j < otherLen; i++, j++)
        {
            dst[i] = src[j];
        }

        dst[newLen] = '\0';
        return MakeError(Error::Success);
    }

private:
    AStringBase(const AStringBase& other) = delete;
    AStringBase& operator=(const AStringBase& other) = delete;

    Vector<char, PoolType> Buf;
};

using AString = AStringBase<>;
using AStringPtr =  SharedPtr<AString>;

}