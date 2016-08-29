#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>

#define KAPI_GFP_UNKNOWN    0UL
#define KAPI_GFP_ATOMIC     1UL
#define KAPI_GFP_KERNEL     2UL
#define KAPI_GFP_NOFS       3UL
#define KAPI_GFP_NOIO       4UL
#define KAPI_GFP_USER       5UL

struct kernel_api
{
    void *(*kmalloc)(size_t size, unsigned long mem_flag);
    void (*kfree)(void *ptr);
    void (*printk)(const char *fmt, ...);
    void (*bug_on)(bool condition);
    void* (*atomic_create)(int value, unsigned long mem_flag);
    void (*atomic_inc)(void *atomic);
    bool (*atomic_dec_and_test)(void *atomic);
    int (*atomic_read)(void *atomic);
    void (*atomic_delete)(void *atomic);
    void (*atomic_set)(void *atomic, int value);
    void* (*completion_create)(unsigned long mem_flag);
    void (*completion_delete)(void *completion);
    void (*completion_wait)(void *completion);
    void (*completion_complete)(void *completion);
    void (*completion_complete_all)(void *completion);
    void* (*task_create)(int (*task_fn)(void *data), void *data,
                         const char *name);
    void (*task_wakeup)(void *task);
    void (*task_stop)(void *task);
    bool (*task_should_stop)(void);
    void (*task_get)(void *task);
    void (*task_put)(void *task);
    void *(*task_current)(void);
    int (*task_get_id)(void *task);
    void (*msleep)(unsigned int msecs);
    void* (*spinlock_create)(unsigned long mem_flag);
    void (*spinlock_delete)(void* spinlock);
    void (*spinlock_lock)(void* spinlock);
    void (*spinlock_unlock)(void* spinlock);
    unsigned long (*get_symbol_address)(const char *symbol);
    long (*probe_kernel_read)(void *dst, const void *src, size_t size);
    long (*probe_kernel_write)(void *dts, const void *src, size_t size);
    void (*smp_call_function)(void (*function)(void *info), void *info,
                              bool wait);
    void (*get_online_cpus)(void);
    void (*put_online_cpus)(void);
    void (*preempt_disable)(void);
    void (*preempt_enable)(void);
    int (*smp_processor_id)(void);

    void *(*rwsem_create)(unsigned long mem_flag);
    void (*rwsem_down_write)(void *sem);
    void (*rwsem_up_write)(void *sem);
    void (*rwsem_down_read)(void *sem);
    void (*rwsem_up_read)(void *sem);
    void (*rwsem_delete)(void *sem);
};

#ifdef __cplusplus
}
#endif
