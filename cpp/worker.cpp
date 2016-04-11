#include "worker.h"

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

bool Worker::ExecuteAndWait(RunnableRef task, NTSTATUS& status)
{
    if (!Exec(task))
        return false;

    task.get()->Wait();
    status = task.get()->GetStatus();
    return true;
}

NTSTATUS Worker::Run(const Threadable& thread)
{
    while (!thread.IsStopping())
    {
        TaskEvent.Wait();
        RunnableRef task;
        {
            AutoLock lock(Lock);
            if (!TaskList.IsEmpty())
            {
                task = TaskList.PopHead();
            }
        }
        if (task.get())
            task->Exec(thread);
    }

    return STATUS_SUCCESS;
}

Worker::Worker(NTSTATUS& status)
    : Thread(*this, status), Stopping(false)
{
}

Worker::~Worker()
{
    Stopping = true;
    Thread.Stop();
    TaskEvent.Set();
    Thread.StopAndWait();

    RunnableRef task;
    AutoLock lock(Lock);
    while (!TaskList.IsEmpty())
    {
        task = TaskList.PopHead();
        task.get()->Cancel();
    }
}
