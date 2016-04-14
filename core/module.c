#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

#include <stdarg.h>

#include "base.h"
#include "kapi_internal.h"

#include "../cpp/ext.h"

MODULE_LICENSE("GPL");

static int __init kcpp_init(void)
{
    int err;

    PRINTK("loading\n");
    err = kapi_init();
    if (err)
        goto out;

    err = cpp_init(kapi_get());
    if (err)
    {
        kapi_deinit();
        goto out;
    }
out:
    PRINTK("loaded err %d\n", err);
    return err;
}

static void __exit kcpp_exit(void)
{
    PRINTK("exiting\n");
    cpp_deinit();
    kapi_deinit();
    PRINTK("exited\n");
    return;
}

module_init(kcpp_init);
module_exit(kcpp_exit);
