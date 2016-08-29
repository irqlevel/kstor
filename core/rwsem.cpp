#include "rwsem.h"

RWSem::RWSem()
{
    get_kapi()->rwsem_init(&Lock);
}

void RWSem::Acquire()
{
    get_kapi()->rwsem_down_write(&Lock);
}

void RWSem::Release()
{
    get_kapi()->rwsem_up_write(&Lock);
}

void RWSem::AcquireShared()
{
    get_kapi()->rwsem_down_read(&Lock);
}

void RWSem::ReleaseShared()
{
    get_kapi()->rwsem_up_read(&Lock);
}

RWSem::~RWSem()
{
}
