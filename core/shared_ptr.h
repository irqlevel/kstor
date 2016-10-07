#pragma once

#include "atomic.h"
#include "error.h"
#include "trace.h"
#include "bug.h"

template<class T>
class SharedPtr
{
public:
    SharedPtr()
    {
        trace(255, "this %p", this);
        Ptr = nullptr;
        Counter = nullptr;
    }
    SharedPtr(T *ptr)
    {
        trace(255, "this %p Ptr %p", this, ptr);
        Ptr = nullptr;
        Counter = nullptr;
        Atomic* counter = new Atomic(0);
        if (counter == nullptr)
        {
            return;
        }
        Ptr = ptr;
        Counter = counter;
        Counter->Inc();
    }
    SharedPtr(const SharedPtr<T>& other)
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
    SharedPtr(const SharedPtr<T>& other, Error& err)
    {
        if (err != Error::Success)
            return;

        Counter = other.Counter;
        Ptr = other.Ptr;
        trace(255, "this %p Ptr %p Counter %p other %p",
              this, Ptr, Counter, &other);
        if (Counter != nullptr)
        {
            Acquire();
        }
    }
    SharedPtr(SharedPtr<T>&& other)
    {
        Counter = other.Counter;
        Ptr = other.Ptr;
        trace(255, "this %p Ptr %p Counter %p", this, Ptr, Counter);
        other.Counter = nullptr;
        other.Ptr = nullptr;
    }
    SharedPtr<T>& operator=(const SharedPtr<T>& other)
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
    SharedPtr<T>& operator=(SharedPtr<T>&& other)
    {
        Reset();
        Counter = other.Counter;
        Ptr = other.Ptr;
        trace(255, "this %p Ptr %p Counter %p", this, Ptr, Counter);
        other.Counter = nullptr;
        other.Ptr = nullptr;
        return *this;
    }
    T* Get() const
    {
        return Ptr;
    }
    T& operator*()
    {
        return *Get();
    }
    T* operator->()
    {
        return Get();
    }
    int GetCounter()
    {
        return Counter->Get();
    }
    virtual ~SharedPtr()
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
            BUG_ON(!Counter);
            if (Ptr)
                delete Ptr;
            delete Counter;
        }
    }
    T* Ptr;
    Atomic* Counter;
};
