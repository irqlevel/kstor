#include "worker.h"
#include "auto_lock.h"
#include "trace.h"

namespace Core
{

Worker::Worker()
    : Stopping(false), Running(false)
{
}

bool Worker::Execute(RunnablePtr task)
{
    if (Stopping || !Running)
        return false;

    AutoLock lock(Lock);
    if (Stopping || !Running)
        return false;

    bool result = TaskList.AddTail(task);
    if (result)
        TaskEvent.SetAll();
    return result;
}

bool Worker::ExecuteAndWait(RunnablePtr task, Error& err)
{
    if (!Execute(task))
        return false;

    task->Wait();
    err = task->GetStatus();
    return true;
}

Error Worker::Run(const Threadable& thread)
{
    while (!thread.IsStopping())
    {
        trace(255, "Run");
        TaskEvent.Wait(10);
        RunnablePtr task;
        {
            AutoLock lock(Lock);
            trace(255, "Locked");
            if (!TaskList.IsEmpty())
            {
                task = TaskList.Head();
                TaskList.PopHead();
            }
            trace(255, "De-locking");
        }
        if (task.Get())
        {
            task->Execute(thread);
        }
    }

    trace(255, "Stopping");
    return Error::Success;
}

Worker::Worker(const AString& name, Error& err)
    : Stopping(false), Running(false)
{
    if (!err.Ok())
        return;

    AString localName(name, err);
    if (!err.Ok())
        return;
    Name = Memory::Move(localName);

    err = WorkerThread.Start(Name, this);
    if (!err.Ok())
        return;

    Running = true;
    trace(255, "create %p", this);
}

Worker::~Worker()
{
    trace(255, "die %p", this);
    Stopping = true;
    if (!Running)
        return;

    WorkerThread.Stop();
    TaskEvent.SetAll();
    WorkerThread.Wait();

    bool bHasTasks;
    do {
        RunnablePtr task;
        {
            AutoLock lock(Lock);
            bHasTasks = !TaskList.IsEmpty();
            if (bHasTasks)
            {
                task = TaskList.Head();
                TaskList.PopHead();
                bHasTasks = !TaskList.IsEmpty();
            }
        }
        if (task.Get())
        {
            task->Cancel();
        }
    } while (bHasTasks);
}


}