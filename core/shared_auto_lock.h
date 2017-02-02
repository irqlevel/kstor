#pragma once

#include "shared_lockable.h"
#include "trace.h"

namespace Core
{

class SharedAutoLock
{
public:
    SharedAutoLock(SharedLockable& lock)
        : Lock(lock)
    {
        Lock.AcquireShared();
        trace(255, "Lock 0x%p acquired", &Lock);
    }

    virtual ~SharedAutoLock()
    {
        Lock.ReleaseShared();
        trace(255, "Lock 0x%p released", &Lock);
    }
private:
    SharedAutoLock(const SharedAutoLock& other) = delete;
    SharedAutoLock(SharedAutoLock&& other) = delete;
    SharedAutoLock() = delete;
    SharedAutoLock& operator=(const SharedAutoLock& other) = delete;
    SharedAutoLock& operator=(SharedAutoLock&& other) = delete;
    SharedLockable& Lock;
};

}