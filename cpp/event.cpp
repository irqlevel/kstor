#include "event.h"

Event::Event()
{
}

Event::Event(int& err)
{
    if (err)
    {
        return;
    }

    Completion = get_kapi()->completion_create();
    if (!Completion)
    {
        err = E_NO_MEM;
        return;
    }
    err = E_OK;
}

void Event::Set()
{
    PRINTF("Set %p\n", this);
    get_kapi()->completion_complete(Completion);
}

void Event::SetAll()
{
    PRINTF("SetAll %p\n", this);
    get_kapi()->completion_complete_all(Completion);
}

void Event::Wait()
{
    PRINTF("Wait %p %u\n", this, *((unsigned int *)Completion));
    get_kapi()->completion_wait(Completion);
}

Event::~Event()
{
    if (Completion)
        get_kapi()->completion_delete(Completion);
}
