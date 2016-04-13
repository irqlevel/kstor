#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include <stdarg.h>

#include <cpp/main.h>

MODULE_LICENSE("GPL");

static char* truncate_file_name(char* file_name)
{
    char* base;

    base = strrchr(file_name, '/');
    if (base)
        return ++base;
    else
        return file_name;
}

#define PRINTK(fmt, ...)    \
    printk(KCPP ": p%d %s,%d %s:" fmt, current->pid, \
                truncate_file_name(__FILE__),__LINE__,\
                __PRETTY_FUNCTION__, ##__VA_ARGS__)

static void *kapi_malloc(size_t size)
{
    return kmalloc(size, GFP_NOIO);
}

static void kapi_free(void *ptr)
{
    return kfree(ptr);
}

static void kapi_printf(const char *fmt, ...)
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

static void *kapi_atomic_create(int value)
{
    atomic_t *atomic;

    atomic = kapi_malloc(sizeof(*atomic));
    if (!atomic)
        return NULL;
    atomic_set(atomic, value);
    return atomic;
}

static void kapi_atomic_delete(void *atomic)
{
    kapi_free(atomic);
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

static void *kapi_completion_create(void)
{
    struct completion *comp;

    comp = kapi_malloc(sizeof(*comp));
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
    kapi_free(comp);
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

static void* kapi_spinlock_create(void)
{
    spinlock_t *lock;

    lock = kapi_malloc(sizeof(*lock));
    if (!lock)
        return NULL;
    spin_lock_init(lock);
    return lock;
}

static void kapi_spinlock_delete(void *lock)
{
    kapi_free(lock);
}

static void kapi_spinlock_lock(void *lock)
{
    spin_lock((spinlock_t *)lock);
}

static void kapi_spinlock_unlock(void *lock)
{
    spin_unlock((spinlock_t *)lock);
}

static struct kernel_api g_kapi =
{
    .malloc = kapi_malloc,
    .free = kapi_free,
    .printf = kapi_printf,
    .bug_on = kapi_bug_on,
    .atomic_create = kapi_atomic_create,
    .atomic_delete = kapi_atomic_delete,
    .atomic_inc = kapi_atomic_inc,
    .atomic_dec_and_test = kapi_atomic_dec_and_test,
    .atomic_read = kapi_atomic_read,
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
    .spinlock_unlock = kapi_spinlock_unlock
};

static int __init kcpp_init(void)
{
    int err;

    PRINTK("loading\n");
    err = cpp_init(&g_kapi);
    PRINTK("loaded err %d\n", err);
    return err;
}

static void __exit kcpp_exit(void)
{
    PRINTK("exiting\n");
    cpp_deinit();
    PRINTK("exited\n");
    return;
}

module_init(kcpp_init);
module_exit(kcpp_exit);