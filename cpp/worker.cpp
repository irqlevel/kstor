#include "worker.h"
#include "auto_lock.h"

bool Worker::Execute(RunnableRef task)
{
    if (Stopping)
        return false;

    AutoLock lock(Lock);
    if (Stopping)
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
        PRINTF("Run\n");
        TaskEvent.Wait();
        RunnableRef task;
        {
            AutoLock lock(Lock);
            PRINTF("Locked\n");
            if (!TaskList.IsEmpty())
            {
                task = TaskList.PopHead();
            }
            PRINTF("De-locking\n");
        }
        if (task.get())
            task->Execute(thread);
    }

    return E_OK;
}

Worker::Worker(int& err)
    : Runnable(err), Lock(err), TaskEvent(err), Stopping(false),
      WorkerThread(RunnableRef(this), err)
{
}

Worker::~Worker()
{
    PRINTF("die %p\n", this);
    Stopping = true;
    WorkerThread.Stop();
    TaskEvent.Set();
    WorkerThread.StopAndWait();

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
