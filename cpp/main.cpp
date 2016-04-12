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

class TWorker : public Runnable
{
public:
    TWorker(int& err)
        : Runnable(err)
    {
    }
    virtual ~TWorker()
    {
    }
    int Run(const Threadable& thread)
    {
        PRINTF("Hello\n");
        return E_OK;
    }
};

void test_worker()
{
    int err = E_OK;

    WorkerRef worker = WorkerRef(new Worker(err));
    if (!worker.get() || err)
        return;

    err = E_OK;
   if (!worker->ExecuteAndWait(RunnableRef(new TWorker(err)), err))
        return;

    PRINTF("waited err %d\n", err);
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
