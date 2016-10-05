#include <core/shared_ptr.h>
#include <core/list_entry.h>
#include <core/list.h>
#include <core/auto_lock.h>
#include <core/spinlock.h>
#include <core/worker.h>
#include <core/trace.h>
#include <core/vector.h>
#include <core/astring.h>
#include <core/hash_table.h>
#include <core/smp.h>
#include <core/error.h>
#include <core/page.h>
#include <core/block_device.h>
#include <core/bio.h>
#include <core/random.h>

class TJob : public Runnable
{
public:
    TJob()
    {
        trace(1,"job %p ctor", this);
    }
    virtual ~TJob()
    {
        trace(1,"job %p dtor", this);
    }
    Error Run(const Threadable& thread)
    {
        trace(1,"Hello from job %p", this);
        return Error::Success;
    }
};

void test_worker()
{
    trace(1,"Test worker!!!");
    Worker w;

    Error err = Error::Success;
    WorkerRef worker(new (Memory::PoolType::Atomic) Worker(err));
    if (!worker.get() || err != Error::Success)
        return;

    RunnableRef job(new TJob());
    if (!job.get() || err != Error::Success)
        return;

    if (!worker->ExecuteAndWait(job, err))
        return;

    trace(1,"Waited job err %d", err.GetCode());
}

void test_vector()
{
    Vector<char> v(Memory::PoolType::Atomic);

    v.PushBack('a');
    v.PushBack('b');

    trace(1, "v[0]=%c v[1]=%c", v[0], v[1]);
}

void test_astring()
{
    Error err;
    AString s("blabla", Memory::PoolType::Atomic, err);
    if (err != Error::Success)
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
    Error err;
    HashTable<int, int> ht(Memory::PoolType::Kernel, 256, err, IntCmp, IntHash);
    if (err != Error::Success)
        return err.GetCode();

    ht.Insert(2, 11);
    ht.Insert(3, -7);
    trace(1, "ht[2]=%d", ht.Get(2));
    trace(1, "ht[3]=%d", ht.Get(3));
    ht.Remove(3);
    trace(1, "ht[3] exists %d", ht.Exists(3));

    return err.GetCode();
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

void test_atomic()
{
    Atomic s;
    s.Set(1);
}

void test_page()
{
    Error err;
    Page p(Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(1, "can't allocate page");
        return;
    }

    trace(1, "page map 0x%p", p.Map());
    p.Unmap();
}

void test_bdev()
{
    Error err;
    AString name("/dev/loop10", Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate string, err %d", err.GetCode());
        return;
    }

    BlockDevice bdev(name, err);
    if (err != Error::Success)
    {
        trace(0, "Can't create bdev, err %d", err.GetCode());
        return;
    }
}

void test_bio()
{
    Error err;
    AString name("/dev/loop10", Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate string, err %d", err.GetCode());
        return;
    }

    BlockDevice bdev(name, err);
    if (err != Error::Success)
    {
        trace(0, "Can't create bdev, err %d", err.GetCode());
        return;
    }

    Random rng(err);
    if (err != Error::Success)
    {
        trace(0, "Can't create rng, err %d", err.GetCode());
        return;
    }

    Page page(Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate page");
        return;
    }

    err = page.FillRandom(rng);
    if (err != Error::Success)
    {
        trace(0, "Can't fill page by rng, err %d", err.GetCode());
        return;
    }

    {
        Bio bio(bdev, page, 0, err, true);
        if (err != Error::Success)
        {
            trace(0, "Can't init Bio, err %d", err.GetCode());
            return;
        }

        bio.Submit();
        bio.Wait();
        trace(1, "Bio write result %d", bio.GetError().GetCode());
    }

    Page pageRead(Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate page");
        return;
    }

    {
        Bio bio(bdev, pageRead, 0, err);
        if (err != Error::Success)
        {
            trace(0, "Can't init Bio, err %d", err.GetCode());
            return;
        }

        bio.Submit();
        bio.Wait();
        trace(1, "Bio read result %d", bio.GetError().GetCode());
    }

    trace(1, "Compare pages content result %d", page.CompareContent(pageRead));
}

void test_random()
{
    Error err;
    Random rng(err);
    if (err != Error::Success)
    {
        trace(0, "Can't create rng, err %d", err.GetCode());
        return;
    }

    for (int i = 0; i < 10; i++)
    {
        trace(1, "random[%d]=%lu", i, rng.GetUlong() % 10);
    }
}

void run_tests()
{
    test_worker();
    test_vector();
    test_astring();
    test_hash_table();
    test_smp();
    test_atomic();
    test_page();
    test_bdev();
    test_bio();
    test_random();
}
