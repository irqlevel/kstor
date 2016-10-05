#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/preempt.h>
#include <linux/highmem.h>
#include <linux/bio.h>
#include <linux/version.h>

#include <stdarg.h>

#include "kapi_internal.h"

#include "base.h"
#include "malloc_checker.h"
#include "page_checker.h"

_Static_assert(sizeof(struct kapi_spinlock) >= sizeof(spinlock_t), "Bad size");
_Static_assert(sizeof(struct kapi_atomic) >= sizeof(atomic_t), "Bad size");
_Static_assert(sizeof(struct kapi_completion) >= sizeof(struct completion),
              "Bad size");
_Static_assert(sizeof(struct kapi_rwsem) >= sizeof(struct rw_semaphore),
              "Bad size");

static gfp_t kapi_get_gfp_flags(unsigned long pool_type)
{
    gfp_t flags;

    switch (pool_type)
    {
    case KAPI_POOL_TYPE_ATOMIC:
        flags = GFP_ATOMIC;
        break;
    case KAPI_POOL_TYPE_KERNEL:
        flags = GFP_KERNEL;
        break;
    case KAPI_POOL_TYPE_NOIO:
        flags = GFP_NOIO;
        break;
    case KAPI_POOL_TYPE_NOFS:
        flags = GFP_NOFS;
        break;
    case KAPI_POOL_TYPE_USER:
        flags = GFP_USER;
        break;
    default:
        flags = 0;
        BUG();
        break;
    }

    return flags;
}

void *kapi_kmalloc_gfp(size_t size, gfp_t flags)
{
#ifdef __MALLOC_CHECKER__
    return malloc_checker_kmalloc(size, flags);
#else
    return kmalloc(size, flags);
#endif
}

static void *kapi_kmalloc(size_t size, unsigned long pool_type)
{
    return kapi_kmalloc_gfp(size, kapi_get_gfp_flags(pool_type));
}

void kapi_kfree(void *ptr)
{
#ifdef __MALLOC_CHECKER__
    malloc_checker_kfree(ptr);
#else
    kfree(ptr);
#endif
}

static void kapi_memset(void* ptr, int c, size_t size)
{
    memset(ptr, c, size);
}

static void kapi_printk(const char *fmt, ...)
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

static void *kapi_atomic_create(int value, unsigned long pool_type)
{
    atomic_t *atomic;

    atomic = kapi_kmalloc(sizeof(*atomic), pool_type);
    if (!atomic)
        return NULL;
    atomic_set(atomic, value);
    return atomic;
}

static void kapi_atomic_delete(void *atomic)
{
    kapi_kfree(atomic);
}

static void kapi_atomic_inc(void *atomic)
{
    atomic_inc((atomic_t *)atomic);
}

static bool kapi_atomic_dec_and_test(void *atomic)
{
    return atomic_dec_and_test((atomic_t *)atomic) ? true : false;
}

static int kapi_atomic_read(void *atomic)
{
    return atomic_read((atomic_t *)atomic);
}

static void kapi_atomic_set(void *atomic, int value)
{
    atomic_set((atomic_t *)atomic, value);
}

static void *kapi_completion_create(unsigned long pool_type)
{
    struct completion *comp;

    comp = kapi_kmalloc(sizeof(*comp), pool_type);
    if (!comp)
        return NULL;
    init_completion(comp);
    return comp;
}

static void kapi_completion_init(void *comp)
{
    init_completion((struct completion *)comp);
}

static void kapi_completion_wait(void *comp)
{
    wait_for_completion((struct completion *)comp);
}

static void kapi_completion_complete(void *comp)
{
    complete((struct completion *)comp);
}

static void kapi_completion_complete_all(void *comp)
{
    complete_all((struct completion *)comp);
}

static void kapi_completion_delete(void *comp)
{
    kapi_kfree(comp);
}

static void *kapi_task_create(int (*task_fn)(void *data), void *data,
                              const char *name)
{
    struct task_struct *thread;

    thread = kthread_create(task_fn, data, "%s", name);
    if (IS_ERR(thread))
        return NULL;
    return thread;
}

static void kapi_task_wakeup(void *task)
{
    wake_up_process((struct task_struct *)task);
}

static void kapi_task_stop(void *task)
{
    kthread_stop((struct task_struct *)task);
}

static bool kapi_task_should_stop(void)
{
    return kthread_should_stop();
}

static void kapi_task_get(void *task)
{
    get_task_struct((struct task_struct *)task);
}

static void kapi_task_put(void *task)
{
    put_task_struct((struct task_struct *)task);
}

static void *kapi_task_current(void)
{
    return current;
}

static int kapi_task_get_id(void *task)
{
    return ((struct task_struct *)task)->pid;
}

static void kapi_msleep(unsigned int msecs)
{
    msleep(msecs);
}

static void* kapi_spinlock_create(unsigned long pool_type)
{
    spinlock_t *lock;

    lock = kapi_kmalloc(sizeof(*lock), pool_type);
    if (!lock)
        return NULL;
    spin_lock_init(lock);
    return lock;
}

static void kapi_spinlock_init(void *lock)
{
    spin_lock_init((spinlock_t *)lock);
}

static void kapi_spinlock_delete(void *lock)
{
    kapi_kfree(lock);
}

static void kapi_spinlock_lock(void *lock)
{
    spin_lock((spinlock_t *)lock);
}

static void kapi_spinlock_unlock(void *lock)
{
    spin_unlock((spinlock_t *)lock);
}

static unsigned long kapi_get_symbol_address(const char *symbol)
{
    return kallsyms_lookup_name(symbol);
}

static long kapi_probe_kernel_read(void *dst, const void *src, size_t size)
{
    return probe_kernel_read(dst, src, size);
}

static long kapi_probe_kernel_write(void *dst, const void *src, size_t size)
{
    return probe_kernel_write(dst, src, size);
}

static void kapi_smp_call_function(void (*function)(void *info), void *info,
                                   bool wait)
{
    smp_call_function(function, info, (wait) ? 1 : 0);
}

static void kapi_get_online_cpus(void)
{
    get_online_cpus();
}

static void kapi_put_online_cpus(void)
{
    put_online_cpus();
}

static void kapi_preempt_disable(void)
{
    preempt_disable();
}

static void kapi_preempt_enable(void)
{
    preempt_enable();
}

static int kapi_smp_processor_id(void)
{
    return smp_processor_id();
}

static void *kapi_rwsem_create(unsigned long pool_type)
{
    struct rw_semaphore *sem;

    sem = kapi_kmalloc(sizeof(*sem), pool_type);
    if (!sem)
        return NULL;

    init_rwsem(sem);
    return sem;
}

static void kapi_rwsem_init(void *sem)
{
    init_rwsem((struct rw_semaphore *)sem);
}

static void kapi_rwsem_down_write(void *sem)
{
    down_write((struct rw_semaphore *)sem);
}

static void kapi_rwsem_up_write(void *sem)
{
    up_write((struct rw_semaphore *)sem);
}

static void kapi_rwsem_down_read(void *sem)
{
    down_read((struct rw_semaphore *)sem);
}

static void kapi_rwsem_up_read(void *sem)
{
    up_read((struct rw_semaphore *)sem);
}

static void kapi_rwsem_delete(void *sem)
{
    kapi_kfree(sem);
}

static void *kapi_alloc_page(unsigned long pool_type)
{
    struct page *page;

#ifdef __PAGE_CHECKER__
    page = page_checker_alloc_page(kapi_get_gfp_flags(pool_type));
#else
    page = alloc_page(kapi_get_gfp_flags(pool_type));
#endif
    if (!page)
    {
        return NULL;
    }
    return page;
}

static void *kapi_map_page(void *page)
{
    return kmap((struct page *)page);
}

static void kapi_unmap_page(void *page)
{
    kunmap((struct page *)page);
}

static void* kapi_map_page_atomic(void* page)
{
    struct page* page_ = (struct page*)page;
    return kmap_atomic(page_);
}

static void kapi_unmap_page_atomic(void* va)
{
    kunmap_atomic(va);
}

static void kapi_free_page(void *page)
{
#ifdef __PAGE_CHECKER__
    page_checker_free_page((struct page *)page);
#else
    put_page((struct page *)page);
#endif
}

static int kapi_get_page_size(void)
{
    return PAGE_SIZE;
}

static fmode_t kapi_get_fmode_by_mode(int mode)
{
    fmode_t fmode = 0;

    if (mode & KAPI_BDEV_MODE_READ)
        fmode |= FMODE_READ;
    if (mode & KAPI_BDEV_MODE_WRITE)
        fmode |= FMODE_WRITE;
    if (mode & KAPI_BDEV_MODE_EXCLUSIVE)
        fmode |= FMODE_EXCL;

    return fmode;
}

static int kapi_bdev_get_by_path(const char *path, int mode, void *holder, void **pbdev)
{
    struct block_device *bdev;

    bdev = blkdev_get_by_path(path, kapi_get_fmode_by_mode(mode), holder);
    if (IS_ERR(bdev))
    {
        return PTR_ERR(bdev);
    }

    *pbdev = bdev;
    return 0;
}

static void kapi_bdev_put(void *bdev, int mode)
{
    blkdev_put(bdev, kapi_get_fmode_by_mode(mode));
}

static void* kapi_alloc_bio(int page_count)
{
    struct bio* bio = bio_alloc(GFP_NOIO, page_count);
    int i;

    if (!bio)
    {
        return NULL;
    }
    bio->bi_iter.bi_size = 0;
    bio->bi_iter.bi_sector = 0;
    bio->bi_vcnt = 0;
    for (i = 0; i < bio->bi_max_vecs; i++)
    {
        bio->bi_io_vec[i].bv_page = NULL;
        bio->bi_io_vec[i].bv_offset = 0;
        bio->bi_io_vec[i].bv_len = 0;
    }

    return bio;
}

struct kapi_bio_private
{
    void* bio;
    void* priv;
    void (*bio_end_io)(void* bio, int err);
};

static struct kapi_bio_private* kapi_get_bio_private_(struct bio* bio)
{
    struct bio* bio_ = (struct bio*)bio;
    struct kapi_bio_private* priv = (struct kapi_bio_private*)bio_->bi_private;

    if (!priv)
    {
        return NULL;
    }

    BUG_ON(priv->bio != bio);
    return priv;
}

static void kapi_free_bio(void* bio)
{
    struct bio* bio_ = (struct bio*)bio;
    struct kapi_bio_private* priv;
    int i;

    priv = kapi_get_bio_private_(bio_);
    if (priv)
    {
        kapi_kfree(priv);
        bio_->bi_private = NULL;
    }

    for (i = 0; i < bio_->bi_vcnt; i++)
    {
        struct page* page = bio_->bi_io_vec[i].bv_page;

        BUG_ON(!page);
        put_page(page);
    }

    bio_put(bio_);
}

static int kapi_set_bio_page(void* bio, int page_index, void* page, int offset, int len)
{
    struct bio* bio_ = (struct bio*)bio;
    struct page* page_ = (struct page*)page;

    if (page_index >= bio_->bi_max_vecs ||
        bio_->bi_vcnt >= bio_->bi_max_vecs ||
        bio_->bi_io_vec[page_index].bv_page ||
        !page_ || len == 0 || (offset + len) > PAGE_SIZE)
    {
        return -EINVAL;
    }

    get_page(page_);
    bio_->bi_io_vec[page_index].bv_page = page_;
    bio_->bi_io_vec[page_index].bv_offset = offset;
    bio_->bi_io_vec[page_index].bv_len = len;
    bio_->bi_vcnt++;
    bio_->bi_iter.bi_size+= len;

    return 0;
}

static struct kapi_bio_private* kapi_get_or_create_bio_private(struct bio* bio)
{
    struct kapi_bio_private* priv = bio->bi_private;

    if (priv)
    {
        BUG_ON(priv->bio != bio);
        return priv;
    }

    priv = (struct kapi_bio_private*)kapi_kmalloc_gfp(sizeof(*priv), GFP_NOIO);
    if (!priv)
    {
        return NULL;
    }

    memset(priv, 0, sizeof(*priv));
    priv->bio = bio;
    bio->bi_private = priv;
    return priv;
}

static void __kapi_io_end_bio(struct bio* bio, int err)
{
    struct kapi_bio_private* priv = kapi_get_bio_private_(bio);

    if (priv && priv->bio_end_io)
    {
        priv->bio_end_io(bio, err);
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
static void kapi_io_end_bio(struct bio *bio)
{
    __kapi_io_end_bio(bio, bio->bi_error);
}
#else
static void kapi_io_end_bio(struct bio *bio, int err)
{
    __kapi_io_end_bio(bio, err);
}
#endif

static int kapi_set_bio_end_io(void* bio, void (*bio_end_io)(void* bio, int err), void* priv)
{
    struct bio* bio_ = (struct bio*)bio;
    struct kapi_bio_private* priv_;

    priv_ = kapi_get_or_create_bio_private(bio_);
    if (!priv_)
    {
        return -ENOMEM;
    }
    priv_->priv = priv;
    priv_->bio_end_io = bio_end_io;
    bio_->bi_end_io = kapi_io_end_bio;
    return 0;
}

static void kapi_set_bio_bdev(void* bio, void* bdev)
{
    struct bio* bio_ = (struct bio*)bio;

    bio_->bi_bdev = (struct block_device*)bdev;
}

static void kapi_set_bio_rw(void* bio, int rw)
{
    struct bio* bio_ = (struct bio*)bio;

    if (rw & KAPI_BIO_READ)
    {
        bio_->bi_rw |= READ;
    }

    if (rw & KAPI_BIO_WRITE)
    {
        bio_->bi_rw |= WRITE;
    }

    if (rw & KAPI_BIO_FLUSH)
    {
        bio_->bi_rw |= REQ_FLUSH;
    }

    if (rw & KAPI_BIO_FUA)
    {
        bio_->bi_rw |= REQ_FUA;
    }
}

static void kapi_set_bio_flags(void* bio, int flags)
{
    struct bio* bio_ = (struct bio*)bio;

    bio_->bi_flags |= flags;
}

static void kapi_set_bio_position(void* bio, unsigned long long sector)
{
    struct bio* bio_ = (struct bio*)bio;

    bio_->bi_iter.bi_sector = sector;
}

static void* kapi_get_bio_private(void* bio)
{
    struct bio* bio_ = (struct bio*)bio;
    struct kapi_bio_private* priv_ = kapi_get_bio_private_(bio_);
    if (!priv_)
    {
        return NULL;
    }

    return priv_->priv;
}

static void kapi_submit_bio(void* bio)
{
    struct bio* bio_ = (struct bio*)bio;

    submit_bio(bio_->bi_rw, bio_);
}

static struct kernel_api g_kapi =
{
    .kmalloc = kapi_kmalloc,
    .kfree = kapi_kfree,
    .memset = kapi_memset,

    .printk = kapi_printk,
    .bug_on = kapi_bug_on,

    .atomic_create = kapi_atomic_create,
    .atomic_delete = kapi_atomic_delete,
    .atomic_inc = kapi_atomic_inc,
    .atomic_dec_and_test = kapi_atomic_dec_and_test,
    .atomic_read = kapi_atomic_read,
    .atomic_set = kapi_atomic_set,

    .completion_create = kapi_completion_create,
    .completion_init = kapi_completion_init,
    .completion_delete = kapi_completion_delete,
    .completion_wait = kapi_completion_wait,
    .completion_complete = kapi_completion_complete,
    .completion_complete_all = kapi_completion_complete_all,

    .task_create = kapi_task_create,
    .task_wakeup = kapi_task_wakeup,
    .task_stop = kapi_task_stop,
    .task_should_stop = kapi_task_should_stop,
    .task_put = kapi_task_put,
    .task_get = kapi_task_get,
    .task_get_id = kapi_task_get_id,
    .task_current = kapi_task_current,
    .msleep = kapi_msleep,

    .spinlock_create = kapi_spinlock_create,
    .spinlock_init = kapi_spinlock_init,
    .spinlock_delete = kapi_spinlock_delete,
    .spinlock_lock = kapi_spinlock_lock,
    .spinlock_unlock = kapi_spinlock_unlock,

    .get_symbol_address = kapi_get_symbol_address,
    .probe_kernel_read = kapi_probe_kernel_read,
    .probe_kernel_write = kapi_probe_kernel_write,

    .smp_call_function = kapi_smp_call_function,
    .get_online_cpus = kapi_get_online_cpus,
    .put_online_cpus = kapi_put_online_cpus,
    .preempt_disable = kapi_preempt_disable,
    .preempt_enable = kapi_preempt_enable,
    .smp_processor_id = kapi_smp_processor_id,

    .rwsem_create = kapi_rwsem_create,
    .rwsem_init = kapi_rwsem_init,
    .rwsem_down_write = kapi_rwsem_down_write,
    .rwsem_up_write = kapi_rwsem_up_write,
    .rwsem_down_read = kapi_rwsem_down_read,
    .rwsem_up_read = kapi_rwsem_up_read,
    .rwsem_delete = kapi_rwsem_delete,

    .alloc_page = kapi_alloc_page,
    .map_page = kapi_map_page,
    .unmap_page = kapi_unmap_page,
    .free_page = kapi_free_page,
    .get_page_size = kapi_get_page_size,
    .map_page_atomic = kapi_map_page_atomic,
    .unmap_page_atomic = kapi_unmap_page_atomic,

    .bdev_get_by_path = kapi_bdev_get_by_path,
    .bdev_put = kapi_bdev_put,

    .alloc_bio = kapi_alloc_bio,
    .free_bio = kapi_free_bio,
    .set_bio_page = kapi_set_bio_page,
    .set_bio_end_io = kapi_set_bio_end_io,
    .set_bio_bdev = kapi_set_bio_bdev,
    .set_bio_rw = kapi_set_bio_rw,
    .set_bio_flags = kapi_set_bio_flags,
    .set_bio_position = kapi_set_bio_position,
    .get_bio_private = kapi_get_bio_private,
    .submit_bio = kapi_submit_bio
};

int kapi_init(void)
{
    int r;

#ifdef __MALLOC_CHECKER__
    r = malloc_checker_init();
#else
    r = 0;
#endif
    if (r)
        return r;

#ifdef __PAGE_CHECKER__
    r = page_checker_init();
#else
    r = 0;
#endif
    if (r)
        goto deinit_malloc_checker;

    return 0;

deinit_malloc_checker:
#ifdef __MALLOC_CHECKER__
    malloc_checker_deinit();
#endif
    return r;
}

void kapi_deinit(void)
{
#ifdef __PAGE_CHECKER__
    page_checker_deinit();
#endif
#ifdef __MALLOC_CHECKER__
    malloc_checker_deinit();
#endif
}

struct kernel_api *kapi_get(void)
{
    return &g_kapi;
}
