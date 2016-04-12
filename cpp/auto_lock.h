#pragma once

#include "main.h"
#include "lockable.h"

class AutoLock
{
public:
    AutoLock(Lockable& lock)
    {
        Lock = &lock;
        Lock->Acquire();
    }

    virtual ~AutoLock()
    {
        Lock->Release();
    }
private:
    Lockable* Lock;
};
