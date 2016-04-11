#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>

#include <cpp/main.h>

#include <stdarg.h>

MODULE_LICENSE("GPL");

#define PRINTK(fmt, ...)	\
    printk("kcpp: " fmt, ##__VA_ARGS__)

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

    atomic = kmalloc(sizeof(*atomic), GFP_NOIO);
    if (!atomic)
        return NULL;
    atomic_set(atomic, value);
    return atomic;
}

static void kapi_atomic_delete(void *atomic)
{
    kfree(atomic);
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
    .atomic_read = kapi_atomic_read
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
