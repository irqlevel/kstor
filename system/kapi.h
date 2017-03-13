#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

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

    void (*memset)(void* ptr, int c, size_t size);
    int (*memcmp)(const void* ptr1, const void* ptr2, size_t size);
    void (*memcpy)(void* dst, const void* src, size_t size);

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
    void (*completion_wait_timeout)(void *completion, unsigned long timeout);
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
    void (*spinlock_lock_irqsave)(void* spinlock, unsigned long* irq_flags);
    void (*spinlock_unlock_irqrestore)(void* spinlock, unsigned long irq_flags);

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
    void* (*map_page_atomic)(void* page);
    void (*unmap_page_atomic)(void* va);

    void (*free_page)(void *page);
    int (*get_page_size)(void);

    int (*bdev_get_by_path)(const char *path, int mode, void *holder, void **pbdev);
    void (*bdev_put)(void *bdev, int mode);
    unsigned long long (*bdev_get_size)(void* bdev);

    void* (*alloc_bio)(int page_count, unsigned long pool_type);
    void (*free_bio)(void* bio);
    int (*set_bio_page)(void* bio, int page_index, void* page, int offset, int len);
    int (*set_bio_end_io)(void* bio, void (*bio_end_io)(void* bio, int err), void* priv);
    void (*set_bio_bdev)(void* bio, void* bdev);
    void (*set_bio_flags)(void* bio, int flags);
    void (*set_bio_position)(void* bio, unsigned long long sector);
    void* (*get_bio_private)(void* bio);
    void (*submit_bio)(void* bio, unsigned int op, unsigned int op_flags);

    int (*vfs_file_open)(const char *path, int flags, void** file);
    int (*vfs_file_write)(void* file, const void* buf, unsigned long len, unsigned long long offset);
    int (*vfs_file_read)(void* file, void* buf, unsigned long len, unsigned long long offset);
    int (*vfs_file_sync)(void* file);
    void (*vfs_file_close)(void* file);

    int (*misc_dev_register)(const char* name, void* context,
        long (*ioctl)(void* context, unsigned int code, unsigned long arg),
        void** dev);
    void (*misc_dev_unregister)(void* dev);

    int (*copy_to_user)(void* dst, const void* src, unsigned long size);
    int (*copy_from_user)(void* dst, const void* src, unsigned long size);

    unsigned long long (*get_time)(void);

    void (*trace_msg)(const char* fmt, va_list args);

    void (*set_bit)(long nr, unsigned long *addr);
    void (*clear_bit)(long nr, unsigned long *addr);
    int (*test_and_clear_bit)(long nr, unsigned long *addr);
    int (*test_and_set_bit)(long nr, unsigned long *addr);

    int (*sock_connect)(void **sockp, const char *host, unsigned short port);
    int (*sock_listen)(void **sockp, const char *host, int port, int backlog);
    void (*sock_release)(void *sockp);
    int (*sock_send)(void *sockp, const void *buf, int len);
    int (*sock_recv)(void *sockp, void *buf, int len);
    int (*sock_accept)(void **newsockp, void *sockp);
    void (*sock_abort_accept)(void *sockp);

    unsigned int (*le32_to_cpu)(unsigned int value);
    unsigned int (*cpu_to_le32)(unsigned int value);

    unsigned long long (*le64_to_cpu)(unsigned long long value);
    unsigned long long (*cpu_to_le64)(unsigned long long value);

    void (*get_random_bytes)(void *buf, int len);

};

#define KAPI_BDEV_MODE_READ         0x1
#define KAPI_BDEV_MODE_WRITE        0x2
#define KAPI_BDEV_MODE_EXCLUSIVE    0x4

#define KAPI_BIO_OP_READ    1
#define KAPI_BIO_OP_WRITE   2 
#define KAPI_BIO_OP_FLUSH   3
#define KAPI_BIO_OP_DISCARD 4

#define KAPI_BIO_REQ_FUA        0x1
#define KAPI_BIO_REQ_SYNC       0x2
#define KAPI_BIO_REQ_PREFLUSH   0x4

#define KAPI_VFS_FILE_RDONLY    0x1
#define KAPI_VFS_FILE_WRONLY    0x2
#define KAPI_VFS_FILE_RDWR      0x4
#define KAPI_VFS_FILE_CREAT     0x8
#define KAPI_VFS_FILE_EXCL      0x10

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
