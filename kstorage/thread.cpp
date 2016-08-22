#include "thread.h"
#include "trace.h"

Thread::Thread()
    : Routine(nullptr), Task(nullptr), Stopping(false), Running(false)
{
}

Thread::Thread(Runnable* routine, int& err)
   : Routine(nullptr), Task(nullptr), Stopping(false), Running(false),
     CompEvent(err)
{
    if (err)
    {
        return;
    }
    Start(routine, err);
}

int Thread::StartRoutine(void* context)
{
    Thread* thread = static_cast<Thread*>(context);
    return thread->ExecuteRoutine();
}

int Thread::ExecuteRoutine()
{
    int err = Routine->Execute(*this);
    CompEvent.SetAll();
    return err;
}

void Thread::Start(Runnable* routine, int& err)
{
    if (err)
    {
        return;
    }
    if (!routine)
    {
        err = E_INVAL;
        return;
    }
    if (err)
    {
        return;
    }
    Routine = routine;
    Task = get_kapi()->task_create(&Thread::StartRoutine, this,
				   "kstorage-thread");
    if (!Task)
    {
        err = E_NO_MEM;
        return;
    }
    Running = true;
    get_kapi()->task_get(Task);
    get_kapi()->task_wakeup(Task);
    err = E_OK;
}

void Thread::Stop()
{
    Stopping = true;
    trace(1, "Set thread %p stopping", this, Stopping);
}

bool Thread::IsStopping() const
{
    trace(1, "Is thread %p stopping %d", this, Stopping);
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
