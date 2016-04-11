#pragma once

#include "main.h"
#include "atomic.h"
#include "error.h"

template<class T>
class shared_ptr
{
public:
    shared_ptr()
    {
        Ptr = nullptr;
        Counter = nullptr;
    }
    shared_ptr(T *ptr)
    {
        Ptr = nullptr;
        int err = E_OK;
        Counter = new Atomic(0, err);
        if (Counter == nullptr)
        {
            return;
        }
        if (err)
        {
            delete Counter;
            return;
        }
        Ptr = ptr;
        Counter->Inc();
    }
    shared_ptr(const shared_ptr<T>& other)
    {
        Counter = other.Counter;
        Ptr = other.Ptr;
        if (Counter != nullptr)
        {
            Acquire();
        }
    }
    shared_ptr<T>& operator=(const shared_ptr<T>& other)
    {
        Reset();
        Counter = other.Counter;
        Ptr = other.Ptr;
        if (Counter != nullptr)
        {
            Acquire();
        }
        return *this;
    }
    shared_ptr<T>& operator=(const shared_ptr<T>&& other)
    {
        Reset();
        Counter = other.Counter;
        Ptr = other.Ptr;
        other.Counter = nullptr;
        other.Ptr = nullptr;
        return *this;
    }
    T* get()
    {
        return Ptr;
    }
    T& operator*()
    {
        return *get();
    }
    T* operator->()
    {
        return get();
    }
    int GetCounter()
    {
        return Counter->Get();
    }
    virtual ~shared_ptr()
    {
        Reset();
    }
    void Reset()
    {
        Release();
        Counter = nullptr;
        Ptr = nullptr;
    }
private:
    void Acquire()
    {
        Counter->Inc();
    }
    void Release()
    {
        if (Counter == nullptr)
            return;
        if (Counter->DecAndTest())
        {
            delete Ptr;
            delete Counter;
        }
    }
    T* Ptr;
    Atomic* Counter;
};
