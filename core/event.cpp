#include "event.h"
#include "trace.h"

Event::Event()
{
    get_kapi()->completion_init(&Completion);
}

void Event::Set()
{
    trace(255, "Set %p", this);
    get_kapi()->completion_complete(&Completion);
}

void Event::SetAll()
{
    trace(255, "SetAll %p", this);
    get_kapi()->completion_complete_all(&Completion);
}

void Event::Wait()
{
    trace(255, "Wait %p %u", this, *((unsigned int *)&Completion));
    get_kapi()->completion_wait(&Completion);
}

Event::~Event()
{
}
