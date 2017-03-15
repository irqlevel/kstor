#pragma once

#include "error.h"
#include "memory.h"
#include "astring.h"

namespace Core
{

class Task
{
public:
    Task(int pid, Error& err);
    Error DumpStack(AString& result);
    virtual ~Task();
private:
    void* TaskPtr;
};

}