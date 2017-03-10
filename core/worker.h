#pragma once

#include "list.h"
#include "thread.h"
#include "event.h"
#include "runnable.h"
#include "spinlock.h"
#include "shared_ptr.h"
#include "error.h"
#include "memory.h"
#include "astring.h"

namespace Core
{

class Worker : public Runnable
{
public:
    Worker();
    Worker(const AString& name, Error& err);
    virtual ~Worker();
    bool Execute(RunnablePtr task);
    bool ExecuteAndWait(RunnablePtr task, Error& err);
    Error Run(const Threadable& thread);
private:
    Worker(const Worker& other) = delete;
    Worker(Worker&& other) = delete;
    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other) = delete;

    bool Stopping;
    bool Running;

    SpinLock Lock;
    LinkedList<RunnablePtr, Memory::PoolType::Kernel> TaskList;
    Event TaskEvent;
    Thread WorkerThread;
    AString Name;
};

typedef SharedPtr<Worker, Memory::PoolType::Kernel> WorkerPtr;

}