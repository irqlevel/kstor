#pragma once

#include "main.h"
#include "list.h"
#include "thread.h"
#include "event.h"

class Worker
{
public:
    Worker(int& err);
    virtual ~Worker();
    bool Execute(RunnableRef task);
    bool ExecuteAndWait(RunnableRef task, int& err);
    int Run(const Threadable& thread);
private:
    LinkedList<RunnableRef> TaskList;
    Thread Thread;
    bool Stopping;
};
