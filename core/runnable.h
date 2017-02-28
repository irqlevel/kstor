#pragma once

#include "error.h"
#include "threadable.h"
#include "shared_ptr.h"
#include "event.h"
#include "memory.h"

namespace Core
{

class Runnable
{
public:
    virtual Error Run(const Threadable& thread) = 0;

    Runnable()
    {
        SetStatus(Error::NotExecuted);
    }

    Error Execute(const Threadable& thread)
    {
        Error err = Run(thread);
        SetStatus(err);
        CompleteEvent.SetAll();
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
        CompleteEvent.SetAll();
    }

    virtual ~Runnable()
    {
    }

private:
    Runnable(const Runnable& other) = delete;
    Runnable(Runnable&& other) = delete;
    Runnable& operator=(const Runnable& other) = delete;
    Runnable& operator=(Runnable&& other) = delete;

    void SetStatus(const Error& err)
    {
        Status = err;
    }
    Error Status;
    Event CompleteEvent;
};

typedef SharedPtr<Runnable, Memory::PoolType::Kernel> RunnablePtr;

}