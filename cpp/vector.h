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

    int GetSize()
    {
        return Size;
    }

    int GetCapacity()
    {
        return Capacity;
    }

    T& operator[](int index)
    {
        KBUG_ON(index < 0 || index >= Size);
        return Arr[index];
    }

    bool Reserve(int capacity)
    {
        if (capacity <= Capacity)
            return true;

        T* newArr = new (MemoryType) T[capacity];
        if (!newArr)
            return false;

        if (Arr)
        {
            for (int i = 0; i < Size; i++)
            {
                newArr[i] = util::move(Arr[i]);
            }
            delete[] Arr;
        }
        Arr = newArr;
        Capacity = capacity;
        return true;
    }

    bool push_back(T&& e)
    {
        if (Size == Capacity)
        {
            if (!Reserve(Size + 1))
                return false;
        }
        Arr[Size++] = util::move(e);
        return true;
    }

    bool push_back(T& e)
    {
        if (Size == Capacity)
        {
            if (!Reserve(Size + 1))
                return false;
        }
        Arr[Size++] = e;
        return true;
    }

    virtual ~Vector()
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
    void Release()
    {
        if (Arr)
            delete[] Arr;
    }

    T* Arr;
    int Size;
    int Capacity;
    MemType MemoryType;
};
