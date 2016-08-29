#pragma once

#include "kapi.h"

class Event
{
public:
    Event();
    void Set();
    void SetAll();
    void Wait();
    virtual ~Event();

private:
    Event(const Event& other) = delete;
    Event(Event&& other) = delete;
    Event& operator=(const Event& other) = delete;
    Event& operator=(Event&& other) = delete;
    struct kapi_completion Completion;
};
