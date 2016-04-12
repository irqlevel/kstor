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
        PRINTF("this %p\n", this);
        Ptr = nullptr;
        Counter = nullptr;
    }
    shared_ptr(T *ptr)
    {
        PRINTF("this %p Ptr %p\n", this, ptr);
        if (ptr == nullptr)
            return;

        Ptr = nullptr;
        Counter = nullptr;
        int err = E_OK;
        Atomic* counter = new Atomic(0, err);
        if (counter == nullptr)
        {
            return;
        }
        if (err)
        {
            delete counter;
            return;
        }
        Ptr = ptr;
        Counter = counter;
        Counter->Inc();
    }
    shared_ptr(const shared_ptr<T>& other)
    {
        Counter = other.Counter;
        Ptr = other.Ptr;
        PRINTF("this %p Ptr %p Counter %p other %p\n",
               this, Ptr, Counter, &other);
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
        PRINTF("this %p Ptr %p Counter %p\n", this, Ptr, Counter);
        if (Counter != nullptr)
        {
            Acquire();
        }
        return *this;
    }
    shared_ptr<T>& operator=(shared_ptr<T>&& other)
    {
        Reset();
        Counter = other.Counter;
        Ptr = other.Ptr;
        PRINTF("this %p Ptr %p Counter %p\n", this, Ptr, Counter);
        other.Counter = nullptr;
        other.Ptr = nullptr;
        return *this;
    }
    T* get() const
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
        PRINTF("this %p Ptr %p Counter %p %d\n",
               this, Ptr, Counter, Counter->Get());
    }
    void Release()
    {
        if (Counter == nullptr)
            return;
        if (Counter->DecAndTest())
        {
            PRINTF("this %p Deleting Ptr %p Counter %p %d\n",
                   this, Ptr, Counter, Counter->Get());
            KBUG_ON(!Ptr);
            KBUG_ON(!Counter);
            delete Ptr;
            delete Counter;
        }
    }
    T* Ptr;
    Atomic* Counter;
};
