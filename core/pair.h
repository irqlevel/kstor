#pragma once

#include "memory.h"

namespace Core
{

template<typename T1, typename T2>
class Pair
{
public:
    Pair()
        : First()
        , Second()
    {
    }

    Pair(const T1& first, const T2& second)
        : First(first)
        , Second(second)
    {
    }

    virtual ~Pair()
    {
    }

    Pair(const Pair& other)
    {
        First = other.First;
        Second = other.Second;
    }

    Pair& operator=(const Pair& other)
    {
        if (this != &other)
        {
            First = other.First;
            Second = other.Second;
        }
        return *this;
    }

    Pair(Pair&& other)
    {
        First = Memory::Move(other.First);
        Second = Memory::Move(other.Second);
    }

    Pair& operator=(Pair&& other)
    {
        if (this != &other)
        {
            First = Memory::Move(other.First);
            Second = Memory::Move(other.Second);
        }
        return *this;
    }

    T1 First;
    T2 Second;

private:
};

}