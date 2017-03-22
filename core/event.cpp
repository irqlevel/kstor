#include "event.h"
#include "trace.h"

namespace Core
{

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
    get_kapi()->completion_wait(&Completion);
}

void Event::Wait(unsigned long timeout)
{
    get_kapi()->completion_wait_timeout(&Completion, timeout);
}

void Event::Reset()
{
}

Event::~Event()
{
}

}