#pragma once

#include "vector.h"
#include "shared_ptr.h"
#include "error.h"
#include "memory.h"

template <Memory::PoolType PoolType>
class AStringBase
{
public:
    AStringBase()
    {        
    }

    AStringBase(const char* s, Error& err)
    {
        if (err != Error::Success)
        {
            return;
        }

        size_t len = Memory::StrLen(s);
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
        if (!Buf.GetSize())
        {
            return 0;
        }

        return Buf.GetSize() - 1;
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
