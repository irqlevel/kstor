#include "thread.h"
#include "trace.h"

namespace Core
{

Thread::Thread()
    : Routine(nullptr), Task(nullptr), Stopping(false), Running(false)
{
}

Thread::Thread(const AString& name, Runnable* routine, Error& err)
   : Routine(nullptr), Task(nullptr), Stopping(false), Running(false)
{
    if (!err.Ok())
        return;

    err = Start(name, routine);
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

Core::Error Thread::Start(const AString &name, Runnable* routine)
{
    Core::Error err;

    if (routine == nullptr)
    {
        err.SetInvalidValue();
        return err;
    }

    AString localName(name, err);
    if (!err.Ok())
    {
        return err;
    }

    Name = Memory::Move(localName);
    Routine = routine;
    Task = get_kapi()->task_create(&Thread::StartRoutine, this, Name.GetBuf());
    if (!Task)
    {
        err.SetNoMemory();
        return err;
    }
    Running = true;
    get_kapi()->task_get(Task);
    get_kapi()->task_wakeup(Task);
    return err;
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