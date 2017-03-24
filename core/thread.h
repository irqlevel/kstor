#pragma once

#include "runnable.h"
#include "threadable.h"
#include "event.h"
#include "error.h"
#include "astring.h"

namespace Core
{

class Thread : public Threadable
{
public:
    Thread();
    Thread(const AString& name, Runnable *routine, Error& err);
    Error Start(const AString& name, Runnable *routine);
    void Stop();
    void Wait();
    void StopAndWait();
    bool IsStopping() const;
    void *GetId() const;
    virtual ~Thread();
    static void Sleep(int milliseconds);

private:
    Thread(const Thread& other) = delete;
    Thread(Thread&& other) = delete;
    Thread& operator=(const Thread& other) = delete;
    Thread& operator=(Thread&& other) = delete;

    static int StartRoutine(void* context);
    Error ExecuteRoutine();
    Runnable* Routine;
    void* Task;
    bool Stopping;
    bool Running;
    Event CompEvent;
    AString Name;
};

}