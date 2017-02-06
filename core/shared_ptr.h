#pragma once

#include "atomic.h"
#include "error.h"
#include "trace.h"
#include "bug.h"
#include "memory.h"

namespace Core
{

template<typename T, Memory::PoolType PoolType>
class SharedPtr
{
public:

    class ObjectReference
    {
    public:
        Atomic Counter;
        T* Object;

        ObjectReference(T* object)
            : Object(nullptr)
        {
            Counter.Set(1);
            Object = object;
        }

        virtual ~ObjectReference()
        {
        }

    private:
        ObjectReference() = delete;
        ObjectReference(const ObjectReference& other) = delete;
        ObjectReference(ObjectReference&& other) = delete;
        ObjectReference& operator=(const ObjectReference& other) = delete;
        ObjectReference& operator=(ObjectReference&& other) = delete;
    };

    SharedPtr()
    {
        ObjectRef = nullptr;
    }

    SharedPtr(T *object)
        : SharedPtr()
    {
        Reset(object);
    }

    SharedPtr(const SharedPtr<T, PoolType>& other)
        : SharedPtr()
    {
        ObjectRef = other.ObjectRef;
        if (ObjectRef != nullptr)
        {
            ObjectRef->Counter.Inc();
        }
    }

    SharedPtr(SharedPtr<T, PoolType>&& other)
        : SharedPtr()
    {
        ObjectRef = other.ObjectRef;
        other.ObjectRef = nullptr;
    }

    SharedPtr<T, PoolType>& operator=(const SharedPtr<T, PoolType>& other)
    {
        if (this != &other)
        {
            Reset(nullptr);
            ObjectRef = other.ObjectRef;
            if (ObjectRef != nullptr)
            {
                ObjectRef->Counter.Inc();
            }
        }
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
        return *this;
    }

    T* Get() const
    {
        return (ObjectRef != nullptr) ? ObjectRef->Object : nullptr;
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
        return (ObjectRef != nullptr) ? ObjectRef->Counter.Get() : 0;
    }

    virtual ~SharedPtr()
    {
        Reset(nullptr);
    }

    void Reset(T* object)
    {
        if (ObjectRef != nullptr)
        {
            if (ObjectRef->Counter.DecAndTest())
            {
                trace(255, "SharedPtr 0x%p Delete ObjectRef 0x%p Object 0x%p",
                    this, ObjectRef, ObjectRef->Object);

                delete ObjectRef->Object;
                delete ObjectRef;
            }
        }

        ObjectRef = nullptr;

        if (object != nullptr)
        {
            ObjectRef = new (PoolType) ObjectReference(object);
            if (ObjectRef == nullptr)
            {
                trace(0, "SharedPtr 0x%p can't allocate object reference", this);
                return;
            }

            trace(255, "SharedPtr 0x%p New ObjectRef 0x%p Object 0x%p",
                    this, ObjectRef, ObjectRef->Object);
        }
    }

    void Reset()
    {
        Reset(nullptr);
    }

private:
    ObjectReference* ObjectRef;
};

}