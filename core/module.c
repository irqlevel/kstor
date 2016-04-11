#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

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

static struct kernel_api g_kapi =
{
	.malloc = kapi_malloc,
	.free = kapi_free,
	.printf = kapi_printf, 
	.bug_on = kapi_bug_on
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
