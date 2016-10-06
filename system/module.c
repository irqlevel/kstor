#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>

#include <stdarg.h>

#include "base.h"
#include "kapi_internal.h"

#include <core/init.h>
#include <kstor/init.h>

MODULE_LICENSE("GPL");

static int __init kstor_module_init(void)
{
    int err;

    PRINTK("loading\n");

    err = kapi_init();
    if (err)
        goto out;

    err = core_init(kapi_get());
    if (err)
        goto deinit_kapi;

    err = KStorInit();
    if (err)
        goto deinit_core;

    PRINTK("load success\n");
    return 0;

deinit_core:
    core_deinit();
deinit_kapi:
    kapi_deinit();
out:
    PRINTK("load err %d\n", err);
    return err;
}

static void __exit kstor_module_exit(void)
{
    PRINTK("exiting\n");
    KStorDeinit();
    core_deinit();
    kapi_deinit();
    PRINTK("exited\n");
    return;
}

module_init(kstor_module_init);
module_exit(kstor_module_exit);
