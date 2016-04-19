#pragma once

#include "main.h"
#include "list.h"
#include "thread.h"
#include "event.h"
#include "runnable.h"
#include "spinlock.h"
#include "shared_ptr.h"

class Worker : public Runnable
{
public:
    Worker();
    Worker(int& err);
    virtual ~Worker();
    bool Execute(RunnableRef task);
    bool ExecuteAndWait(RunnableRef task, int& err);
    int Run(const Threadable& thread);
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
