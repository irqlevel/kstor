#pragma once

#include "main.h"
#include "list.h"
#include "thread.h"
#include "event.h"
#include "runnable.h"
#include "spinlock.h"
#include "shared_ptr.h"
#include "error.h"

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
    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other) = delete;

    bool Stopping;
    bool Running;

    SpinLock Lock;
    LinkedList<RunnableRef> TaskList;
    Event TaskEvent;
    Thread WorkerThread;
};

typedef shared_ptr<Worker> WorkerRef;
