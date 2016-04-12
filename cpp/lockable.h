#pragma once

#include "main.h"

class Lockable
{
public:
    virtual void Acquire() = 0;
    virtual void Release() = 0;
};
