#include "main.h"
#include "shared_ptr.h"
#include "list_entry.h"
#include "list.h"
#include "auto_lock.h"
#include "spinlock.h"
#include "worker.h"
#include "trace.h"
#include "vector.h"
#include "astring.h"
#include "hash_table.h"
#include "smp.h"

#include "public.h"

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
        trace(1,"job %p ctor", this);
    }
    virtual ~TJob()
    {
        trace(1,"job %p dtor", this);
    }
    int Run(const Threadable& thread)
    {
        trace(1,"Hello from job %p", this);
        return E_OK;
    }
};

void test_worker()
{
    trace(1,"Test worker!!!");
    Worker w;

    int err = E_OK;
    WorkerRef worker(new (MemType::Atomic) Worker(err));
    if (!worker.get() || err)
        return;

    err = E_OK;
    RunnableRef job(new TJob(err));
    if (!job.get() || err)
        return;

    if (!worker->ExecuteAndWait(job, err))
        return;

    trace(1,"Waited job err %d", err);
}

void test_vector()
{
    Vector<char> v(MemType::Atomic);

    v.PushBack('a');
    v.PushBack('b');

    trace(1, "v[0]=%c v[1]=%c", v[0], v[1]);
}

void test_astring()
{
    int err = E_OK;

    AString s("blabla", MemType::Atomic, err);
    if (err)
        return;

    trace(1, "s init err %d", err);
    trace(1, "s content=%s len=%lu", s.GetBuf(), s.GetLen());
}

int IntCmp(const int& key1, const int& key2)
{
    if (key1 > key2)
        return 1;
    if (key1 < key2)
        return -1;
    return 0;
}

size_t IntHash(const int& key)
{
    return key;
}

int test_hash_table()
{
    int err = 0;
    HashTable<int, int> ht(MemType::Kernel, 256, err, IntCmp, IntHash);
    if (err)
        return err;

    ht.Insert(2, 11);
    ht.Insert(3, -7);
    trace(1, "ht[2]=%d", ht.Get(2));
    trace(1, "ht[3]=%d", ht.Get(3));
    ht.Remove(3);
    trace(1, "ht[3] exists %d", ht.Exists(3));

    return err;
}

void only_one_cpu(void *data)
{
    trace(1, "curr cpu %d", Smp::GetCpuId());
}

int test_smp()
{
    Smp::CallFunctionCurrCpuOnly(only_one_cpu, nullptr);
    return 0;
}

int kstorage_init(struct kernel_api *kapi)
{
    g_kapi = *kapi;
    trace(1, "kstorage_init");

    test_worker();
    test_vector();
    test_astring();
    test_hash_table();
    test_smp();

    trace(1, "kstorage_init completed");
    return 0;
}

void kstorage_deinit(void)
{
    trace(1,"kstorage_deinit");
}
