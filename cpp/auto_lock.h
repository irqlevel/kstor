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
        PRINTF("lock %p acquired\n", Lock);
    }

    virtual ~AutoLock()
    {
        Lock->Release();
        PRINTF("lock %p released\n", Lock);
    }
private:
    Lockable* Lock;
};
