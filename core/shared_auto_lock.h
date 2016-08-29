#pragma once

#include "shared_lockable.h"
#include "trace.h"

class ShareAutoLock
{
public:
    ShareAutoLock(ShareLockable& lock)
    {
        Lock = &lock;
        Lock.AcquireShared();
        trace(255,"lock %p acquired", Lock);
    }

    virtual ~ShareAutoLock()
    {
        Lock.ReleaseShared();
        trace(255,"lock %p released", Lock);
    }
private:
    ShareAutoLock(const ShareAutoLock& other) = delete;
    ShareAutoLock(ShareAutoLock&& other) = delete;
    ShareAutoLock() = delete;
    ShareAutoLock& operator=(const ShareAutoLock& other) = delete;
    ShareAutoLock& operator=(ShareAutoLock&& other) = delete;
    ShareLockable& Lock;
};
