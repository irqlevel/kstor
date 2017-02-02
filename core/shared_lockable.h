#pragma once

namespace Core
{

class SharedLockable
{
public:
    virtual void AcquireShared() = 0;
    virtual void ReleaseShared() = 0;
};

}
