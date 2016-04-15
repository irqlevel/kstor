#pragma once

#include "main.h"
#include "atomic.h"

class Event
{
public:
    Event(int& err, MemType memType = MemType::Kernel);
    Event();
    void Set();
    void SetAll();
    void Wait();
    virtual ~Event();
private:
    void *Completion;
};
