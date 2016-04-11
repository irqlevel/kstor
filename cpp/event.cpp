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
    get_kapi()->completion_complete(Completion);
}

void Event::SetAll()
{
    get_kapi()->completion_complete_all(Completion);
}

void Event::Wait()
{
    get_kapi()->completion_wait(Completion);
}

Event::~Event()
{
    if (Completion)
        get_kapi()->completion_delete(Completion);
}
