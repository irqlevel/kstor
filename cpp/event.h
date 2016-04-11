#pragma once

#include "main.h"

class Event
{
public:
    Event(int& err);
    Event();
    void Set();
    void SetAll();
    void Wait();
    virtual ~Event();
private:
    void *Completion;
};
