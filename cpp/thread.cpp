#include "thread.h"

Thread::Thread()
    : Routine(nullptr), Task(nullptr), Stopping(false)
{
}

Thread::Thread(const RunnableRef routine, int& err)
    : Thread()
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

void Thread::Start(const RunnableRef routine, int& err)
{
    if (err)
    {
        return;
    }
    if (!routine.get())
    {
        err = E_INVAL;
        return;
    }
    CompEvent = Event(err);
    if (err)
    {
        return;
    }
    Routine = routine;
    Task = get_kapi()->task_create(&Thread::StartRoutine, this, "kcpp-thread");
    if (!Task)
    {
        err = E_NO_MEM;
        return;
    }
    get_kapi()->task_get(Task);
    get_kapi()->task_wakeup(Task);
    err = E_OK;
}

void Thread::Stop()
{
    Stopping = true;
}

bool Thread::IsStopping() const
{
    return Stopping;
}

void* Thread::GetId() const
{
    return Task;
}

void Thread::Wait()
{
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
