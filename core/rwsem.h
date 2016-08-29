#pragma once

#include "kapi.h"
#include "lockable.h"
#include "shared_lockable.h"

class RWSem : public Lockable, public SharedLockable
{
public:
    RWSem();
    virtual ~RWSem();

    void Acquire();
    void Release();

    void AcquireShared();
    void ReleaseShared();

private:
    RWSem(const RWSem& other) = delete;
    RWSem(RWSem&& other) = delete;
    RWSem& operator=(const RWSem& other) = delete;
    RWSem& operator=(RWSem&& other) = delete;

    struct kapi_rwsem Lock;
};
