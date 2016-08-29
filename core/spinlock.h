#pragma once

#include "main.h"
#include "lockable.h"
#include "error.h"

class SpinLock : public Lockable
{
public:
    SpinLock();
    SpinLock(Error& err, MemType memType = MemType::Kernel);
    virtual ~SpinLock();
    void Acquire();
    void Release();

    SpinLock(SpinLock&& other);
    SpinLock& operator=(SpinLock&& other);

private:
    SpinLock(const SpinLock& other) = delete;
    SpinLock& operator=(const SpinLock& other) = delete;
    void* Lock;
};
