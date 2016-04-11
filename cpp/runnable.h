#pragma once

#include "main.h"

#include "error.h"
#include "threadable.h"
#include "shared_ptr.h"
#include "event.h"

class Runnable
{
public:
    virtual int Run(const Threadable& thread) = 0;

    Runnable(int& err)
        : CompleteEvent(Event(err))
    {
    }

    int Execute(const Threadable& thread)
    {
        int err = Run(thread);
        SetError(err);
        CompleteEvent.Set();
        return err;
    }

    void Wait()
    {
        CompleteEvent.Wait();
    }

    int GetError()
    {
        return Error;
    }

    void Cancel()
    {
        Error = E_CANCELED;
        CompleteEvent.Set();
    }

    virtual ~Runnable() {}
private:
    void SetError(int err)
    {
        Error = err;
    }
    int Error;
    Event CompleteEvent;
};

typedef shared_ptr<Runnable> RunnableRef;
