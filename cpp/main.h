#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>

#include "error.h"

struct kernel_api
{
    void *(*malloc)(size_t size);
    void (*memcpy)(void *dst, void *src, size_t size);
    void (*free)(void *ptr);
    void (*printf)(const char *fmt, ...);
    void (*bug_on)(bool condition);
    void* (*atomic_create)(int value);
    void (*atomic_inc)(void *atomic);
    bool (*atomic_dec_and_test)(void *atomic);
    int (*atomic_read)(void *atomic);
    void (*atomic_delete)(void *atomic);
    void* (*completion_create)(void);
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
    void (*msleep)(unsigned int msecs);
    void* (*spinlock_create)(void);
    void (*spinlock_delete)(void* spinlock);
    void (*spinlock_lock)(void* spinlock);
    void (*spinlock_unlock)(void* spinlock);
};

int cpp_init(struct kernel_api *kapi);
void cpp_deinit(void);
struct kernel_api *get_kapi(void);

#define KCPP "kcpp"

#define PRINTF(fmt, ...)	\
    get_kapi()->printf(KCPP ": %s(),%d:" fmt, __func__, __LINE__, ##__VA_ARGS__)

#define KBUG_ON(cond)   \
    get_kapi()->bug_on(cond)

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)

#ifdef __cplusplus
}
#endif
