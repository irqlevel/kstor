#pragma once

#include "main.h"
#include "atomic.h"
#include "error.h"
#include "trace.h"

template<class T>
class shared_ptr
{
public:
    shared_ptr()
    {
        trace(255, "this %p", this);
        Ptr = nullptr;
        Counter = nullptr;
    }
    shared_ptr(T *ptr)
    {
        trace(255, "this %p Ptr %p", this, ptr);
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
        trace(255, "this %p Ptr %p Counter %p other %p",
              this, Ptr, Counter, &other);
        if (Counter != nullptr)
        {
            Acquire();
        }
    }
    shared_ptr(shared_ptr<T>&& other)
    {
        Counter = other.Counter;
        Ptr = other.Ptr;
        trace(255, "this %p Ptr %p Counter %p", this, Ptr, Counter);
        other.Counter = nullptr;
        other.Ptr = nullptr;
    }

    shared_ptr<T>& operator=(const shared_ptr<T>& other)
    {
        Reset();
        Counter = other.Counter;
        Ptr = other.Ptr;
        trace(255, "this %p Ptr %p Counter %p", this, Ptr, Counter);
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
        trace(255, "this %p Ptr %p Counter %p", this, Ptr, Counter);
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
        trace(255, "dtor %p", this);
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
        trace(255, "this %p Ptr %p Counter %p %d",
               this, Ptr, Counter, Counter->Get());
    }
    void Release()
    {
        trace(255, "this %p Ptr %p Counter %p %d",
              this, Ptr, Counter, (Counter) ? Counter->Get() : 0);
        if (Counter == nullptr)
            return;
        if (Counter->DecAndTest())
        {
            trace(255, "this %p Deleting Ptr %p Counter %p %d",
                  this, Ptr, Counter, Counter->Get());
            KBUG_ON(!Counter);
            if (Ptr)
                delete Ptr;
            delete Counter;
        }
    }
    T* Ptr;
    Atomic* Counter;
};
