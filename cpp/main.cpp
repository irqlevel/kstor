#include "main.h"
#include "kobj.h"
#include "shared_ptr.h"
#include "list_entry.h"
#include "list.h"
#include "auto_lock.h"
#include "spinlock.h"
#include "worker.h"
#include <memory.h>

struct kernel_api g_kapi;

struct kernel_api *get_kapi(void)
{
    return &g_kapi;
}

class TJob : public Runnable
{
public:
    TJob(int& err)
        : Runnable(err)
    {
        PRINTF("job %p ctor\n", this);
    }
    virtual ~TJob()
    {
        PRINTF("job %p dtor\n", this);
    }
    int Run(const Threadable& thread)
    {
        PRINTF("Hello from job %p\n", this);
        return E_OK;
    }
};

void test_worker()
{
    PRINTF("Test worker!!!\n");
    int err = E_OK;
    WorkerRef worker(new Worker(err));
    if (!worker.get() || err)
        return;

    err = E_OK;
    RunnableRef job(new TJob(err));
    if (!job.get() || err)
        return;

    if (!worker->ExecuteAndWait(job, err))
        return;

    PRINTF("Waited job err %d\n", err);
}

int cpp_init(struct kernel_api *kapi)
{
    memcpy(&g_kapi, kapi, sizeof(*kapi));
    PRINTF("cpp_init\n");

    KObj obj(2);

    KObjRef pobj = KObjRef(new KObj(3));
    PRINTF("pobj %p val %d\n", pobj.get(), pobj->GetValue());

    test_worker();
    return 0;
}

void cpp_deinit(void)
{
    PRINTF("cpp_deinit\n");
}
