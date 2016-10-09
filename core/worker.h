#pragma once

#include "list.h"
#include "thread.h"
#include "event.h"
#include "runnable.h"
#include "spinlock.h"
#include "shared_ptr.h"
#include "error.h"
#include "memory.h"

class Worker : public Runnable
{
public:
    Worker();
    Worker(Error& err);
    virtual ~Worker();
    bool Execute(RunnableRef task);
    bool ExecuteAndWait(RunnableRef task, Error& err);
    Error Run(const Threadable& thread);
private:
    Worker(const Worker& other) = delete;
    Worker(Worker&& other) = delete;
    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other) = delete;

    bool Stopping;
    bool Running;

    SpinLock Lock;
    LinkedList<RunnableRef, Memory::PoolType::Kernel> TaskList;
    Event TaskEvent;
    Thread WorkerThread;
};

typedef SharedPtr<Worker, Memory::PoolType::Kernel> WorkerRef;
