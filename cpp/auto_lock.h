#pragma once

#include "main.h"
#include "lockable.h"
#include "trace.h"

class AutoLock
{
public:
    AutoLock(Lockable& lock)
    {
        Lock = &lock;
        Lock->Acquire();
        trace(255,"lock %p acquired", Lock);
    }

    virtual ~AutoLock()
    {
        Lock->Release();
        trace(255,"lock %p released", Lock);
    }
private:
    Lockable* Lock;
};
