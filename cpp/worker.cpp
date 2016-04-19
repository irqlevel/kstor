#include "worker.h"
#include "auto_lock.h"
#include "trace.h"

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

bool Worker::ExecuteAndWait(RunnableRef task, int& err)
{
    if (!Execute(task))
        return false;

    task.get()->Wait();
    err = task.get()->GetError();
    return true;
}

int Worker::Run(const Threadable& thread)
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
                task = TaskList.PopHead();
            }
            trace(255, "De-locking");
        }
        if (task.get())
            task->Execute(thread);
    }

    trace(255, "Stopping");
    return E_OK;
}

Worker::Worker(int& err)
    : Runnable(err), Stopping(false), Running(false), Lock(err),
      TaskEvent(err), WorkerThread(this, err)
{
    if (err)
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
                task = TaskList.PopHead();
                bHasTasks = !TaskList.IsEmpty();
            }
        }
        if (task.get())
            task->Cancel();
    } while (bHasTasks);
}
