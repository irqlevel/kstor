#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>

#include <stdarg.h>

#include "base.h"
#include "kapi_internal.h"

#include "../kstorage/public.h"

MODULE_LICENSE("GPL");

static int __init kstorage_module_init(void)
{
    int err;

    PRINTK("loading\n");

    err = kapi_init();
    if (err)
        goto out;

    err = kstorage_init(kapi_get());
    if (err)
    {
        kapi_deinit();
        goto out;
    }
out:
    PRINTK("loaded err %d\n", err);
    return err;
}

static void __exit kstorage_module_exit(void)
{
    PRINTK("exiting\n");
    kstorage_deinit();
    kapi_deinit();
    PRINTK("exited\n");
    return;
}

module_init(kstorage_module_init);
module_exit(kstorage_module_exit);
