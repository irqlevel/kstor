#pragma once

#include "atomic.h"
#include "error.h"
#include "trace.h"
#include "bug.h"
#include "memory.h"

namespace Core
{

const int SharedPtrLL = 255;

template<typename T, Memory::PoolType PoolType = Memory::PoolType::Kernel>
class ObjectReference final
{
public:
    ObjectReference(T* object)
        : Object(nullptr)
    {
        Counter.Set(1);
        Object = object;

        trace(SharedPtrLL, "objref 0x%p obj 0x%p ctor", this, Object);

#if defined(__DEBUG__)
        if (Object != nullptr)
            panic(get_kapi()->unique_key_register(Object, this, get_kapi_pool_type(PoolType)) != 0);
#endif

    }

    ~ObjectReference()
    {
        trace(SharedPtrLL, "objref 0x%p dtor", this);

        panic(Object != nullptr);
        panic(Counter.Get() != 0);
    }

    void IncCounter()
    {
        Counter.Inc();
        trace(SharedPtrLL, "objref 0x%p obj 0x%p inc counter %d", this, Object, Counter.Get());
    }

    int GetCounter()
    {
        return Counter.Get();
    }

    void SetObject(T *object)
    {
        if (panic(Object != nullptr))
            return;

        Object = object;

#if defined(__DEBUG__)
        panic(get_kapi()->unique_key_register(Object, this, get_kapi_pool_type(PoolType)) != 0);
#endif

    }

    T* GetObject()
    {
        return Object;
    }

    bool DecCounter()
    {
        if (Counter.DecAndTest())
        {
            trace(SharedPtrLL, "objref 0x%p obj 0x%p dec counter %d", this, Object, Counter.Get());

#if defined(__DEBUG__)
            if (panic(get_kapi()->unique_key_unregister(Object, this) != 0))
                return false;
#endif

            delete Object;
            Object = nullptr;
            return true;
        }
        trace(SharedPtrLL, "objref 0x%p obj 0x%p dec counter %d", this, Object, Counter.Get());

        return false;
    }

private:
    Atomic Counter;
    T* Object;

    ObjectReference() = delete;
    ObjectReference(const ObjectReference& other) = delete;
    ObjectReference(ObjectReference&& other) = delete;
    ObjectReference& operator=(const ObjectReference& other) = delete;
    ObjectReference& operator=(ObjectReference&& other) = delete;
};

template<typename T, Memory::PoolType PoolType = Memory::PoolType::Kernel>
class SharedPtr final
{
public:
    SharedPtr()
    {
        ObjectRef = nullptr;

        trace(SharedPtrLL, "ptr 0x%p ctor obj 0x%p", this, Get());
    }

    SharedPtr(ObjectReference<T, PoolType> *objectRef)
    {
        ObjectRef = objectRef;

        trace(SharedPtrLL, "ptr 0x%p ctor obj 0x%p", this, Get());
    }

    SharedPtr(T *object)
        : SharedPtr()
    {
        Reset(object);

        trace(SharedPtrLL, "ptr 0x%p ctor obj 0x%p", this, Get());
    }

    SharedPtr(const SharedPtr<T, PoolType>& other)
        : SharedPtr()
    {
        ObjectRef = other.ObjectRef;
        if (ObjectRef != nullptr)
        {
            ObjectRef->IncCounter();
        }

        trace(SharedPtrLL, "ptr 0x%p ctor obj 0x%p", this, Get());
    }

    SharedPtr(SharedPtr<T, PoolType>&& other)
        : SharedPtr()
    {
        ObjectRef = other.ObjectRef;
        other.ObjectRef = nullptr;

        trace(SharedPtrLL, "ptr 0x%p ctor obj 0x%p", this, Get());
    }

    SharedPtr<T, PoolType>& operator=(const SharedPtr<T, PoolType>& other)
    {
        if (this != &other)
        {
            Reset(nullptr);
            ObjectRef = other.ObjectRef;
            if (ObjectRef != nullptr)
            {
                ObjectRef->IncCounter();
            }
        }

        trace(SharedPtrLL, "ptr 0x%p op= obj 0x%p", this, Get());

        return *this;
    }

    SharedPtr<T, PoolType>& operator=(SharedPtr<T, PoolType>&& other)
    {
        if (this != &other)
        {
            Reset(nullptr);
            ObjectRef = other.ObjectRef;
            other.ObjectRef = nullptr;
        }

        trace(SharedPtrLL, "ptr 0x%p op= obj 0x%p", this, Get());

        return *this;
    }

    T* Get() const
    {
        return (ObjectRef != nullptr) ? ObjectRef->GetObject() : nullptr;
    }

    T& operator*() const
    {
        return *Get();
    }

    T* operator->() const
    {
        return Get();
    }

    int GetCounter()
    {
        return (ObjectRef != nullptr) ? ObjectRef->GetCounter() : 0;
    }

    ~SharedPtr()
    {
        Reset(nullptr);
    }

    void Reset(T* object)
    {
        trace(SharedPtrLL, "ptr 0x%p reset obj 0x%p new 0x%p", this, Get(), object);

        panic(Get() != nullptr && Get() == object);

        if (ObjectRef != nullptr)
        {
            if (ObjectRef->DecCounter())
            {
                delete ObjectRef;
            }
        }

        ObjectRef = nullptr;

        if (object != nullptr)
        {
            ObjectRef = new (PoolType) ObjectReference<T, PoolType>(object);
            if (ObjectRef == nullptr)
            {
                return;
            }
        }
    }

    void Reset()
    {
        Reset(nullptr);
    }

private:
    ObjectReference<T, PoolType>* ObjectRef;
};

template<typename T, Memory::PoolType PoolType, class... Args>
SharedPtr<T, PoolType> MakeShared(Args&&... args)
{
    ObjectReference<T, PoolType>* objRef = new (PoolType) ObjectReference<T, PoolType>(nullptr);
    if (objRef == nullptr)
        return SharedPtr<T, PoolType>();

    T* object = new (PoolType) T(Memory::Forward<Args>(args)...);
    if (object == nullptr)
    {
        delete objRef;
        return SharedPtr<T, PoolType>();
    }

    objRef->SetObject(object);
    return SharedPtr<T, PoolType>(objRef);
}

}