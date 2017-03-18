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
    using Ptr = SharedPtr<Worker>;

    Worker();
    Worker(const AString& name, Error& err);
    virtual ~Worker();
    bool Execute(const Runnable::Ptr& task);
    bool ExecuteAndWait(const Runnable::Ptr& task, Error& err);
    Error Run(const Threadable& thread);
private:
    Worker(const Worker& other) = delete;
    Worker(Worker&& other) = delete;
    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other) = delete;

    bool Stopping;
    bool Running;

    SpinLock Lock;
    LinkedList<Runnable::Ptr> TaskList;
    Event TaskEvent;
    Thread WorkerThread;
    AString Name;
};

}