#pragma once

#include "lockable.h"
#include "kapi.h"

namespace Core
{

class SpinLock : public Lockable
{
public:
    SpinLock();
    virtual ~SpinLock();
    void Acquire();
    void Release();
    SpinLock(SpinLock&& other);
    SpinLock& operator=(SpinLock&& other);
private:
    SpinLock(const SpinLock& other) = delete;
    SpinLock& operator=(const SpinLock& other) = delete;

    struct kapi_spinlock Lock;
    unsigned long IrqFlags;
    bool Active;
    bool Acquired;
};

}
