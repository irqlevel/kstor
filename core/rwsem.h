#pragma once

#include "main.h"
#include "lockable.h"
#include "share_lockable.h"
#include "error.h"

class RWSem : public Lockable, public ShareLockable
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
