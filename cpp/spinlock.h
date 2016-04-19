#pragma once

#include "main.h"
#include "lockable.h"

class SpinLock : public Lockable
{
public:
    SpinLock(int& err, MemType memType = MemType::Kernel);
    virtual ~SpinLock();
    void Acquire();
    void Release();

    SpinLock(SpinLock&& other);
    SpinLock& operator=(SpinLock&& other);

private:
    SpinLock() = delete;
    SpinLock(const SpinLock& other) = delete;
    SpinLock& operator=(const SpinLock& other) = delete;
    void* Lock;
};
