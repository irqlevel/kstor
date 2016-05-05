#pragma once

#include "main.h"
#include "lockable.h"
#include "share_lockable.h"

class RWSem : public Lockable, public ShareLockable
{
public:
    RWSem();
    RWSem(int& err, MemType memType = MemType::Kernel);
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
