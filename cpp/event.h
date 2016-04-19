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

    Event& operator=(Event&& other);
    Event(Event&& other);

private:
    Event(const Event& other) = delete;
    Event& operator=(const Event& other) = delete;
    void *Completion;
};
