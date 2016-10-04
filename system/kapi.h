#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>

#define KAPI_POOL_TYPE_UNKNOWN    0UL
#define KAPI_POOL_TYPE_ATOMIC     1UL
#define KAPI_POOL_TYPE_KERNEL     2UL
#define KAPI_POOL_TYPE_NOFS       3UL
#define KAPI_POOL_TYPE_NOIO       4UL
#define KAPI_POOL_TYPE_USER       5UL

struct kernel_api
{
    void *(*kmalloc)(size_t size, unsigned long pool_type);
    void (*kfree)(void *ptr);

    void (*printk)(const char *fmt, ...);
    void (*bug_on)(bool condition);

    void* (*atomic_create)(int value, unsigned long pool_type);
    void (*atomic_inc)(void *atomic);
    bool (*atomic_dec_and_test)(void *atomic);
    int (*atomic_read)(void *atomic);
    void (*atomic_delete)(void *atomic);
    void (*atomic_set)(void *atomic, int value);

    void* (*completion_create)(unsigned long pool_type);
    void (*completion_init)(void *completion);
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

    void* (*spinlock_create)(unsigned long pool_type);
    void (*spinlock_init)(void* spinlock);
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

    void *(*rwsem_create)(unsigned long pool_type);
    void (*rwsem_init)(void *sem);
    void (*rwsem_down_write)(void *sem);
    void (*rwsem_up_write)(void *sem);
    void (*rwsem_down_read)(void *sem);
    void (*rwsem_up_read)(void *sem);
    void (*rwsem_delete)(void *sem);

    void *(*alloc_page)(unsigned long pool_type);
    void *(*map_page)(void *page);
    void (*unmap_page)(void *page);
    void (*free_page)(void *page);
    int (*get_page_size)(void);

    int (*bdev_get_by_path)(const char *path, int mode, void *holder, void **pbdev);
    void (*bdev_put)(void *bdev, int mode);

    void* (*alloc_bio)(int page_count);
    void (*free_bio)(void* bio);
    int (*set_bio_page)(void* bio, int page_index, void* page, int offset, int len);
    int (*set_bio_private)(void* bio, void* priv);
    int (*set_bio_end_io)(void* bio, void (*bio_end_io)(void* bio, int err));
    void (*set_bio_bdev)(void* bio, void* bdev);
    void (*set_bio_rw)(void* bio, int rw);
    void (*set_bio_flags)(void* bio, int flags);
    void* (*get_bio_private)(void* bio);
    void (*submit_bio)(void* bio);
};

#define KAPI_BDEV_MODE_READ         0x1
#define KAPI_BDEV_MODE_WRITE        0x2
#define KAPI_BDEV_MODE_EXCLUSIVE    0x4

#define KAPI_BIO_READ 0x1
#define KAPI_BIO_WRITE 0x2
#define KAPI_BIO_FLUSH 0x4
#define KAPI_BIO_FUA 0x8

struct kapi_atomic
{
    unsigned long value;
};

struct kapi_spinlock
{
    unsigned long value;
};

struct kapi_completion
{
    unsigned long value[4];
};

struct kapi_rwsem
{
    unsigned long value[5];
};

#ifdef __cplusplus
}
#endif
