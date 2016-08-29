#pragma once

#include "main.h"
#include "lockable.h"
#include "share_lockable.h"
#include "error.h"

class RWSem : public Lockable, public ShareLockable
{
public:
    RWSem();
    RWSem(Error& err, Memory::PoolType poolType = Memory::PoolType::Kernel);
    virtual ~RWSem();

    void Acquire();
    void Release();

    void AcquireShared();
    void ReleaseShared();

    RWSem(RWSem&& other);
    RWSem& operator=(RWSem&& other);

private:
    RWSem(const RWSem& other) = delete;
    RWSem& operator=(const RWSem& other) = delete;
    void* Lock;
};
