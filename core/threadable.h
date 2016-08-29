#pragma once

#include "main.h"

class Threadable
{
public:
    virtual bool IsStopping() const = 0;
    virtual void* GetId() const = 0;
};
