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
private:
    void* Lock;
};
