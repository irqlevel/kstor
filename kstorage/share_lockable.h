#pragma once

#include "main.h"

class ShareLockable
{
public:
    virtual void AcquireShared() = 0;
    virtual void ReleaseShared() = 0;
};
