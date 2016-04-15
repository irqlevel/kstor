#include "event.h"
#include "trace.h"

Event::Event()
{
}

Event::Event(int& err, MemType memType)
{
    if (err)
    {
        return;
    }

    Completion = get_kapi()->completion_create(get_kapi_mem_flag(memType));
    if (!Completion)
    {
        err = E_NO_MEM;
        return;
    }
    err = E_OK;
}

void Event::Set()
{
    trace(255, "Set %p", this);
    get_kapi()->completion_complete(Completion);
}

void Event::SetAll()
{
    trace(255, "SetAll %p", this);
    get_kapi()->completion_complete_all(Completion);
}

void Event::Wait()
{
    trace(255, "Wait %p %u", this, *((unsigned int *)Completion));
    get_kapi()->completion_wait(Completion);
}

Event::~Event()
{
    if (Completion)
        get_kapi()->completion_delete(Completion);
}
