#pragma once

#include "error.h"

namespace Core
{

class RingBuffer
{
public:
    RingBuffer()
    {
        Reset(0, 0, 0, 0);
    }

    RingBuffer(size_t capacity)
    {
        Reset(0, 0, 0, capacity);
    }

    bool Reset(size_t startIndex, size_t endIndex, size_t size, size_t capacity)
    {
        if (capacity == 0)
            return false;
        if (size >= capacity)
            return false;
        if (startIndex >= capacity)
            return false;
        if (endIndex >= capacity)
            return false;
        if (endIndex >= startIndex)
        {
            if ((endIndex - startIndex) != size)
                return false;
        } else
        {
            if (((endIndex + capacity) - startIndex) != size)
                return false;
        }

        Size = size;
        StartIndex = startIndex;
        EndIndex = endIndex;
        Capacity = capacity;
        return true;
    }

    virtual ~RingBuffer()
    {
    }

    Error PushBack(size_t &position)
    {
        if (Size == Capacity)
            return Error::NoMemory;

        position = EndIndex;
        EndIndex = (EndIndex + 1) % Capacity;
        Size++;
        return Error::Success;
    }

    Error PopFront(size_t &position)
    {
        if (Size == 0)
            return Error::NotFound;
        position = StartIndex;
        StartIndex = (StartIndex + 1) % Capacity;
        Size--;
        return Error::Success;
    }

    size_t GetStartIndex() const
    {
        return StartIndex;
    }

    size_t GetEndIndex() const
    {
        return EndIndex;
    }

    size_t GetSize() const
    {
        return Size;
    }

    size_t GetCapacity() const
    {
        return Capacity;
    }

private:
    size_t StartIndex;
    size_t EndIndex;
    size_t Size;
    size_t Capacity;
};

}