#include "worker.h"
#include "auto_lock.h"
#include "trace.h"

namespace Core
{

Worker::Worker()
    : Stopping(false), Running(false)
{
}

bool Worker::Execute(RunnableRef task)
{
    if (Stopping || !Running)
        return false;

    AutoLock lock(Lock);
    if (Stopping || !Running)
        return false;

    bool result = TaskList.AddTail(task);
    TaskEvent.Set();
    return result;
}

bool Worker::ExecuteAndWait(RunnableRef task, Error& err)
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
        TaskEvent.Wait();
        RunnableRef task;
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

Worker::Worker(Error& err)
    : Stopping(false), Running(false),
      WorkerThread(this, err)
{
    if (err != Error::Success)
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
    TaskEvent.Set();
    WorkerThread.Wait();

    bool bHasTasks;
    do {
        RunnableRef task;
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