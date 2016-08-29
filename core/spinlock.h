#pragma once

#include "main.h"
#include "lockable.h"
#include "error.h"

class SpinLock : public Lockable
{
public:
    SpinLock();
    virtual ~SpinLock();
    void Acquire();
    void Release();

private:
    SpinLock(const SpinLock& other) = delete;
    SpinLock& operator=(const SpinLock& other) = delete;
    SpinLock(SpinLock&& other) = delete;
    SpinLock& operator=(SpinLock&& other) = delete;

    struct kapi_spinlock Lock;
};
