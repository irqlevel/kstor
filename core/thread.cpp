#include "thread.h"
#include "trace.h"

namespace Core
{

Thread::Thread()
    : Routine(nullptr), Task(nullptr), Stopping(false), Running(false)
{
}

Thread::Thread(Runnable* routine, Error& err)
   : Routine(nullptr), Task(nullptr), Stopping(false), Running(false)
{
    Start(routine, err);
}

int Thread::StartRoutine(void* context)
{
    Thread* thread = static_cast<Thread*>(context);
    return thread->ExecuteRoutine().GetCode();
}

Error Thread::ExecuteRoutine()
{
    Error err = Routine->Execute(*this);
    CompEvent.SetAll();
    return err;
}

void Thread::Start(Runnable* routine, Error& err)
{
    if (!err.Ok())
    {
        return;
    }
    if (!routine)
    {
        err.SetInvalidValue();
        return;
    }
    if (!err.Ok())
    {
        return;
    }
    Routine = routine;
    Task = get_kapi()->task_create(&Thread::StartRoutine, this,
				   "kstor-thread");
    if (!Task)
    {
        err.SetNoMemory();
        return;
    }
    Running = true;
    get_kapi()->task_get(Task);
    get_kapi()->task_wakeup(Task);
    err = Error::Success;
}

void Thread::Stop()
{
    Stopping = true;
    trace(4, "Set thread %p stopping", this, Stopping);
}

bool Thread::IsStopping() const
{
    trace(5, "Is thread %p stopping %d", this, Stopping);
    return Stopping;
}

void* Thread::GetId() const
{
    return Task;
}

void Thread::Wait()
{
    if (Running)
        CompEvent.Wait();
}

void Thread::StopAndWait()
{
    Stop();
    Wait();
}

Thread::~Thread()
{
    if (Task)
    {
        StopAndWait();
        get_kapi()->task_put(Task);
    }
}

void Thread::Sleep(int milliseconds)
{
    get_kapi()->msleep(milliseconds);
}

}