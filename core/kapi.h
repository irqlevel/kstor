#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>

#define KAPI_MEM_UNKNOWN    0UL
#define KAPI_MEM_ATOMIC     1UL
#define KAPI_MEM_KERNEL     2UL
#define KAPI_MEM_NOFS       3UL
#define KAPI_MEM_NOIO       4UL
#define KAPI_MEM_USER       5UL

struct kernel_api
{
    void *(*malloc)(size_t size, unsigned long mem_flag);
    void (*memcpy)(void *dst, void *src, size_t size);
    void (*free)(void *ptr);
    void (*printf)(const char *fmt, ...);
    void (*bug_on)(bool condition);
    void* (*atomic_create)(int value, unsigned long mem_flag);
    void (*atomic_inc)(void *atomic);
    bool (*atomic_dec_and_test)(void *atomic);
    int (*atomic_read)(void *atomic);
    void (*atomic_delete)(void *atomic);
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
};

#ifdef __cplusplus
}
#endif
