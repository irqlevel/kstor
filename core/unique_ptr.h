#pragma once

namespace Core
{

template<typename T>
class UniquePtr
{
public:
    UniquePtr(UniquePtr&& other)
        : UniquePtr()
    {
        Reset(other.Release());
    }

    UniquePtr& operator=(UniquePtr&& other)
    {
        if (this != &other)
        {
            Reset(other.Release());
        }
        return *this;
    }

    UniquePtr()
        : Object(nullptr)
    {
    }

    UniquePtr(T* object)
        : UniquePtr()
    {
        Object = object;
    }

    UniquePtr(const UniquePtr& other) = delete;

    UniquePtr& operator=(const UniquePtr& other) = delete;

    void Reset(T* object)
    {
        if (Object != nullptr)
        {
            delete Object;
            Object = nullptr;
        }
        Object = object;
    }

    void Reset()
    {
        Reset(nullptr);
    }

    T* Release()
    {
        T* object = Object;
        Object = nullptr;
        return object;
    }

    virtual ~UniquePtr()
    {
        Reset(nullptr);
    }

    T* Get()
    {
        return Object;
    }

    T& operator*()
    {
        return *Get();
    }

    T* operator->()
    {
        return Get();
    }

private:
    T* Object;
};

}