#pragma once

#include "main.h"

template<class T>
class Vector
{
public:
    Vector(MemType memType)
        : Arr(nullptr), Size(0), Capacity(0),
         MemoryType(memType)
    {
    }

    size_t GetSize() const
    {
        return Size;
    }

    size_t GetCapacity() const
    {
        return Capacity;
    }

    T& operator[](size_t index)
    {
        KBUG_ON(index < 0 || index >= Size);
        return Arr[index];
    }

    bool Reserve(size_t capacity)
    {
        if (capacity <= Capacity)
            return true;

        T* newArr = new (MemoryType) T[capacity];
        if (!newArr)
            return false;

        if (Arr)
        {
            for (size_t i = 0; i < Size; i++)
            {
                newArr[i] = util::move(Arr[i]);
            }
            delete[] Arr;
        }
        Arr = newArr;
        Capacity = capacity;
        return true;
    }

    bool PushBack(T&& e)
    {
        if (Size == Capacity)
        {
            if (!Reserve(Size + 1))
                return false;
        }
        Arr[Size++] = util::move(e);
        return true;
    }

    bool PushBack(const T& e)
    {
        if (Size == Capacity)
        {
            if (!Reserve(2*2*2*2*2*2*2*2*Size + 1))
                return false;
        }
        Arr[Size++] = e;
        return true;
    }

    const T* GetBuf() const
    {
        return Arr;
    }

    virtual ~Vector()
    {
        Release();
    }

    void Clear()
    {
        Release();
    }

    Vector(Vector&& other)
    {
        Arr = other.Arr;
        Size = other.Size;
        Capacity = other.Capacity;
        other.Arr = nullptr;
        other.Size = 0;
        other.Capacity = 0;
    }

    Vector& operator=(Vector&& other)
    {
        Release();
        Arr = other.Arr;
        Size = other.Size;
        Capacity = other.Capacity;
        other.Arr = nullptr;
        other.Size = 0;
        other.Capacity = 0;
        return *this;
    }

private:
    Vector(const Vector& other) = delete;
    Vector& operator=(const Vector& other) = delete;

    void Release()
    {
        if (Arr)
        {
            delete[] Arr;
            Arr = nullptr;
        }
        Size = 0;
        Capacity = 0;
    }

    T* Arr;
    size_t Size;
    size_t Capacity;
    MemType MemoryType;
};
