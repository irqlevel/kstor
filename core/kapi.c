#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/preempt.h>

#include <stdarg.h>

#include "kapi_internal.h"

#include "malloc_checker.h"

static void *kapi_kmalloc(size_t size, unsigned long mem_flag)
{
    gfp_t flags;

    switch (mem_flag)
    {
    case KAPI_MEM_ATOMIC:
        flags = GFP_ATOMIC;
        break;
    case KAPI_MEM_KERNEL:
        flags = GFP_KERNEL;
        break;
    case KAPI_MEM_NOIO:
        flags = GFP_NOIO;
        break;
    case KAPI_MEM_NOFS:
        flags = GFP_NOFS;
        break;
    case KAPI_MEM_USER:
        flags = GFP_USER;
        break;
    default:
        BUG();
    }
#ifdef __MALLOC_CHECKER__
    return malloc_checker_kmalloc(size, flags);
#else
    return kmalloc(size, flags);
#endif
}

static void kapi_kfree(void *ptr)
{
#ifdef __MALLOC_CHECKER__
    malloc_checker_kfree(ptr);
#else
    kfree(ptr);
#endif
}

static void kapi_printk(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}

static void kapi_bug_on(bool condition)
{
    if (condition)
    {
        BUG();
    }
}

static void *kapi_atomic_create(int value, unsigned long mem_flag)
{
    atomic_t *atomic;

    atomic = kapi_kmalloc(sizeof(*atomic), mem_flag);
    if (!atomic)
        return NULL;
    atomic_set(atomic, value);
    return atomic;
}

static void kapi_atomic_delete(void *atomic)
{
    kapi_kfree(atomic);
}

static void kapi_atomic_inc(void *atomic)
{
    atomic_inc((atomic_t *)atomic);
}

static bool kapi_atomic_dec_and_test(void *atomic)
{
    return atomic_dec_and_test((atomic_t *)atomic) ? true : false;
}

static int kapi_atomic_read(void *atomic)
{
    return atomic_read((atomic_t *)atomic);
}

static void kapi_atomic_set(void *atomic, int value)
{
    atomic_set((atomic_t *)atomic, value);
}

static void *kapi_completion_create(unsigned long mem_flag)
{
    struct completion *comp;

    comp = kapi_kmalloc(sizeof(*comp), mem_flag);
    if (!comp)
        return NULL;
    init_completion(comp);
    return comp;
}

static void kapi_completion_wait(void *comp)
{
    wait_for_completion((struct completion *)comp);
}

static void kapi_completion_complete(void *comp)
{
    complete((struct completion *)comp);
}

static void kapi_completion_complete_all(void *comp)
{
    complete_all((struct completion *)comp);
}

static void kapi_completion_delete(void *comp)
{
    kapi_kfree(comp);
}

static void *kapi_task_create(int (*task_fn)(void *data), void *data,
                              const char *name)
{
    struct task_struct *thread;

    thread = kthread_create(task_fn, data, "%s", name);
    if (IS_ERR(thread))
        return NULL;
    return thread;
}

static void kapi_task_wakeup(void *task)
{
    wake_up_process((struct task_struct *)task);
}

static void kapi_task_stop(void *task)
{
    kthread_stop((struct task_struct *)task);
}

static bool kapi_task_should_stop(void)
{
    return kthread_should_stop();
}

static void kapi_task_get(void *task)
{
    get_task_struct((struct task_struct *)task);
}

static void kapi_task_put(void *task)
{
    put_task_struct((struct task_struct *)task);
}

static void *kapi_task_current(void)
{
    return current;
}

static int kapi_task_get_id(void *task)
{
    return ((struct task_struct *)task)->pid;
}

static void kapi_msleep(unsigned int msecs)
{
    msleep(msecs);
}

static void* kapi_spinlock_create(unsigned long mem_flag)
{
    spinlock_t *lock;

    lock = kapi_kmalloc(sizeof(*lock), mem_flag);
    if (!lock)
        return NULL;
    spin_lock_init(lock);
    return lock;
}

static void kapi_spinlock_delete(void *lock)
{
    kapi_kfree(lock);
}

static void kapi_spinlock_lock(void *lock)
{
    spin_lock((spinlock_t *)lock);
}

static void kapi_spinlock_unlock(void *lock)
{
    spin_unlock((spinlock_t *)lock);
}

static unsigned long kapi_get_symbol_address(const char *symbol)
{
    return kallsyms_lookup_name(symbol);
}

static long kapi_probe_kernel_read(void *dst, const void *src, size_t size)
{
    return probe_kernel_read(dst, src, size);
}

static long kapi_probe_kernel_write(void *dst, const void *src, size_t size)
{
    return probe_kernel_write(dst, src, size);
}

static void kapi_smp_call_function(void (*function)(void *info), void *info,
                                   bool wait)
{
    smp_call_function(function, info, (wait) ? 1 : 0);
}

static void kapi_get_online_cpus(void)
{
    get_online_cpus();
}

static void kapi_put_online_cpus(void)
{
    put_online_cpus();
}

static void kapi_preempt_disable(void)
{
    preempt_disable();
}

static void kapi_preempt_enable(void)
{
    preempt_enable();
}

static int kapi_smp_processor_id(void)
{
    return smp_processor_id();
}

static void *kapi_rwsem_create(unsigned long mem_flag)
{
    struct rw_semaphore *sem;

    sem = kapi_kmalloc(sizeof(*sem), mem_flag);
    if (!sem)
        return NULL;

    init_rwsem(sem);
    return sem;
}

static void kapi_rwsem_down_write(void *sem)
{
    down_write((struct rw_semaphore *)sem);
}

static void kapi_rwsem_up_write(void *sem)
{
    up_write((struct rw_semaphore *)sem);
}

static void kapi_rwsem_down_read(void *sem)
{
    down_read((struct rw_semaphore *)sem);
}

static void kapi_rwsem_up_read(void *sem)
{
    up_read((struct rw_semaphore *)sem);
}

static void kapi_rwsem_delete(void *sem)
{
    kapi_kfree(sem);
}

static struct kernel_api g_kapi =
{
    .kmalloc = kapi_kmalloc,
    .kfree = kapi_kfree,
    .printk = kapi_printk,
    .bug_on = kapi_bug_on,
    .atomic_create = kapi_atomic_create,
    .atomic_delete = kapi_atomic_delete,
    .atomic_inc = kapi_atomic_inc,
    .atomic_dec_and_test = kapi_atomic_dec_and_test,
    .atomic_read = kapi_atomic_read,
    .atomic_set = kapi_atomic_set,
    .completion_create = kapi_completion_create,
    .completion_delete = kapi_completion_delete,
    .completion_wait = kapi_completion_wait,
    .completion_complete = kapi_completion_complete,
    .completion_complete_all = kapi_completion_complete_all,
    .task_create = kapi_task_create,
    .task_wakeup = kapi_task_wakeup,
    .task_stop = kapi_task_stop,
    .task_should_stop = kapi_task_should_stop,
    .task_put = kapi_task_put,
    .task_get = kapi_task_get,
    .task_get_id = kapi_task_get_id,
    .task_current = kapi_task_current,
    .msleep = kapi_msleep,
    .spinlock_create = kapi_spinlock_create,
    .spinlock_delete = kapi_spinlock_delete,
    .spinlock_lock = kapi_spinlock_lock,
    .spinlock_unlock = kapi_spinlock_unlock,
    .get_symbol_address = kapi_get_symbol_address,
    .probe_kernel_read = kapi_probe_kernel_read,
    .probe_kernel_write = kapi_probe_kernel_write,
    .smp_call_function = kapi_smp_call_function,
    .get_online_cpus = kapi_get_online_cpus,
    .put_online_cpus = kapi_put_online_cpus,
    .preempt_disable = kapi_preempt_disable,
    .preempt_enable = kapi_preempt_enable,
    .smp_processor_id = kapi_smp_processor_id,
    .rwsem_create = kapi_rwsem_create,
    .rwsem_down_write = kapi_rwsem_down_write,
    .rwsem_up_write = kapi_rwsem_up_write,
    .rwsem_down_read = kapi_rwsem_down_read,
    .rwsem_up_read = kapi_rwsem_up_read,
    .rwsem_delete = kapi_rwsem_delete
};

int kapi_init(void)
{
    int r;

#ifdef __MALLOC_CHECKER__
    r = malloc_checker_init();
#else
    r = 0;
#endif
    return r;
}

void kapi_deinit(void)
{
#ifdef __MALLOC_CHECKER__
    malloc_checker_deinit();
#endif
}

struct kernel_api *kapi_get(void)
{
    return &g_kapi;
}
