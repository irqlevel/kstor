#pragma once

class Lockable
{
public:
    virtual void Acquire() = 0;
    virtual void Release() = 0;
};
