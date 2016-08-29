#pragma once

#include "main.h"

#include "error.h"
#include "threadable.h"
#include "shared_ptr.h"
#include "event.h"

class Runnable
{
public:
    virtual Error Run(const Threadable& thread) = 0;

    Runnable()
    {
    }

    Runnable(Error& err)
        : CompleteEvent(util::move(Event(err)))
    {
    }

    Error Execute(const Threadable& thread)
    {
        Error err = Run(thread);
        SetStatus(err);
        CompleteEvent.Set();
        return err;
    }

    void Wait()
    {
        CompleteEvent.Wait();
    }

    Error GetStatus()
    {
        return Status;
    }

    void Cancel()
    {
        SetStatus(Error::Cancelled);
        CompleteEvent.Set();
    }

    virtual ~Runnable()
    {
    }

private:
    Runnable(const Runnable& other) = delete;
    Runnable& operator=(const Runnable& other) = delete;
    Runnable& operator=(Runnable&& other) = delete;

    void SetStatus(const Error& err)
    {
        Status = err;
    }
    Error Status;
    Event CompleteEvent;
};

typedef shared_ptr<Runnable> RunnableRef;
