#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>

#include <stdarg.h>

#include "base.h"
#include "kapi_internal.h"

#include "../cpp/public.h"

MODULE_LICENSE("GPL");

static int __init kcpp_init(void)
{
    int err;

    PRINTK("loading\n");

    PRINTK("startup_64=0x%lx\n", kallsyms_lookup_name("startup_64"));
    PRINTK("_etext=0x%lx\n", kallsyms_lookup_name("_etext"));
    PRINTK("do_rmdir=0x%lx\n", kallsyms_lookup_name("do_rmdir"));
    PRINTK("kcpp_init=%p\n", kcpp_init);

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
