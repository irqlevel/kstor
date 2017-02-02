#pragma once

#include "vector.h"
#include "shared_ptr.h"
#include "error.h"
#include "memory.h"

namespace Core
{

template <Memory::PoolType PoolType>
class AStringBase
{
public:
    AStringBase()
    {        
    }

    AStringBase(const char* s, size_t len, Error& err)
    {
        if (err != Error::Success)
        {
            return;
        }

        if (!Buf.Reserve(len + 1))
        {
            err = Error::NoMemory;
            return;
        }

        for (size_t i = 0; i < len; i++)
        {
            char c = s[i];
            if (c == '\0')
                break;

            if (!Buf.PushBack(c))
            {
                err = Error::NoMemory;
                Buf.Clear();
                return;
            }
        }

        if (!Buf.PushBack('\0'))
        {
            err = Error::NoMemory;
            Buf.Clear();
            return;
        }

        err = Error::Success;
    }

    AStringBase(const char* s, Error& err)
        : AStringBase(s, Memory::StrLen(s), err)
    {
    }

    AStringBase(const AStringBase& s, Error& err)
        : AStringBase(s.GetBuf(), err)
    {
    }

    virtual ~AStringBase()
    {
    }

    const char* GetBuf() const
    {
        return Buf.GetBuf();
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

        const char* buf = GetBuf();
        const char* otherBuf = other.GetBuf();

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

        for (size_t i = 0; i < GetLen(); i++)
        {
            c = GetBuf()[i];
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

private:
    AStringBase(const AStringBase& other) = delete;
    AStringBase& operator=(const AStringBase& other) = delete;

    Vector<char, PoolType> Buf;
};

typedef AStringBase<Memory::PoolType::Kernel> AString;
typedef SharedPtr<AString, Memory::PoolType::Kernel> AStringRef;

}