#pragma once

template<typename T>
class UniquePtr
{
public:
    UniquePtr(T* object)
    {
        Object = object;
    }

    void Reset(T* object)
    {
        if (Object != nullptr)
        {
            delete Object;
            Object = nullptr;
        }
        Object = object;
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