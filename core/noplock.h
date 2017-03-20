#pragma once

#include "auto_lock.h"
#include "shared_auto_lock.h"

namespace Core
{

class NopLock : public Lockable, public SharedLockable
{
public:
    virtual void Acquire() override
    {
    }
    virtual void Release() override
    {
    }
    virtual void AcquireShared() override
    {
    }
    virtual void ReleaseShared() override
    {
    }
private:
};

}