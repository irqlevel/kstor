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
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/timekeeping.h>

#include <stdarg.h>

#include "kapi_internal.h"

#include "base.h"
#include "malloc_checker.h"
#include "page_checker.h"
#include "trace.h"
#include "ksocket.h"

_Static_assert(sizeof(struct kapi_spinlock) >= sizeof(spinlock_t), "Bad size");
_Static_assert(sizeof(struct kapi_atomic) >= sizeof(atomic_t), "Bad size");
_Static_assert(sizeof(struct kapi_completion) >= sizeof(struct completion),
              "Bad size");
_Static_assert(sizeof(struct kapi_rwsem) >= sizeof(struct rw_semaphore),
              "Bad size");

_Static_assert(sizeof(unsigned int) == 4, "Bad size");

static gfp_t kapi_get_gfp_flags(unsigned long pool_type)
{
    gfp_t flags = 0;

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

static int kapi_memcmp(const void* ptr1, const void* ptr2, size_t size)
{
    return memcmp(ptr1, ptr2, size);
}

static void kapi_memcpy(void* dst, const void* src, size_t size)
{
    memcpy(dst, src, size);
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

static void kapi_completion_wait_timeout(void *comp, unsigned long timeout)
{
    wait_for_completion_timeout((struct completion *)comp, timeout);
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

static void kapi_spinlock_lock_irqsave(void* lock, unsigned long* irq_flags)
{
    unsigned long irq_flags_ = 0;

    spin_lock_irqsave((spinlock_t *)lock, irq_flags_);
    *irq_flags = irq_flags_;
}

static void kapi_spinlock_unlock_irqrestore(void* lock, unsigned long irq_flags)
{
    spin_unlock_irqrestore((spinlock_t *)lock, irq_flags);
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

static unsigned long long kapi_bdev_get_size(void* bdev)
{
    return i_size_read(((struct block_device*)bdev)->bd_inode);
}

static void* kapi_alloc_bio(int page_count, unsigned long pool_type)
{
    struct bio* bio;
    int i;

    bio = bio_alloc(kapi_get_gfp_flags(pool_type), page_count);
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
static unsigned int kapi_get_bio_op(unsigned int op)
{
    switch (op)
    {
    case KAPI_BIO_OP_READ:
        return REQ_OP_READ;
    case KAPI_BIO_OP_WRITE:
        return REQ_OP_WRITE;
    case KAPI_BIO_OP_FLUSH:
        return REQ_OP_FLUSH;
    case KAPI_BIO_OP_DISCARD:
        return REQ_OP_DISCARD;
    default:
        return 0;
    }
}

static unsigned int kapi_get_bio_op_flags(unsigned int op_flags)
{
    unsigned int result;

    result = 0;
    if (op_flags & KAPI_BIO_REQ_FUA)
        result |= REQ_FUA;
    if (op_flags & KAPI_BIO_REQ_SYNC)
        result |= REQ_SYNC;
    if (op_flags & KAPI_BIO_REQ_FLUSH)
        result |= REQ_PREFLUSH;

    return result;
}
#else

static int kapi_get_bio_rw(unsigned int op, unsigned int op_flags)
{
    int rw;

    rw = 0;
    switch (rw)
    {
    case KAPI_BIO_OP_READ:
        rw =  0;
        break;
    case KAPI_BIO_OP_WRITE:
        rw =  REQ_WRITE;
        break;
    case KAPI_BIO_OP_FLUSH:
        rw =  REQ_WRITE | REQ_FLUSH;
        break;
    case KAPI_BIO_OP_DISCARD:
        rw = REQ_WRITE | REQ_DISCARD;
        break;
    default:
        break;
    }

    if (op_flags & KAPI_BIO_REQ_FUA)
        rw |= REQ_FUA;
    if (op_flags & KAPI_BIO_REQ_SYNC)
        rw |= REQ_SYNC;
    if (op_flags & KAPI_BIO_REQ_FLUSH)
        rw |= REQ_FLUSH;

    return rw;
}

#endif

static void kapi_submit_bio(void* bio, unsigned int op, unsigned int op_flags)
{
    struct bio* bio_ = (struct bio*)bio;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    bio_set_op_attrs(bio_, kapi_get_bio_op(op), kapi_get_bio_op_flags(op_flags));
    submit_bio(bio_);
#else
    submit_bio(kapi_get_bio_rw(bio_), bio_);
#endif
}

static int kapi_vfs_file_open(const char *path, int flags, void** file)
{
    struct file* file_;
    int flags_ = 0;

    if (flags & KAPI_VFS_FILE_RDONLY)
    {
        flags_ |= O_RDONLY;
    }
    if (flags & KAPI_VFS_FILE_WRONLY)
    {
        flags_ |= O_WRONLY;
    }
    if (flags_ & KAPI_VFS_FILE_CREAT)
    {
        flags_ |= O_CREAT;
    }
    if (flags_ & KAPI_VFS_FILE_EXCL)
    {
        flags_ |= O_EXCL;
    }

    file_ = filp_open(path, flags_, 0);
    if (IS_ERR(file_))
    {
        return PTR_ERR(file_);
    }
    else
    {
        *file = file_;
        return 0;
    }
}

static int kapi_vfs_file_write(void* file, const void* buf, unsigned long len, unsigned long long offset)
{
    struct file* file_ = (struct file*)file;
    int ret;
    mm_segment_t old_fs;
    unsigned long pos = 0;
    loff_t off = offset;

    old_fs = get_fs();
    set_fs(get_ds());
    while (pos < len) {
            ret = vfs_write(file_, (const char *)buf + pos, len - pos, &off);
            if (ret < 0)
                    goto out;
            if (ret == 0) {
                    ret = -EIO;
                    goto out;
            }
            pos += ret;
    }
    ret = 0;
out:
    set_fs(old_fs);
    return ret;
}

static int kapi_vfs_file_read(void* file, void* buf, unsigned long len, unsigned long long offset)
{
    struct file* file_ = (struct file*)file;
    int ret;
    mm_segment_t old_fs;
    unsigned long pos = 0;
    loff_t off = offset;

    old_fs = get_fs();
    set_fs(get_ds());
    while (pos < len) {
            ret = vfs_read(file_, (char *)buf + pos, len - pos, &off);
            if (ret < 0) {
                    goto out;
            }

            if (ret == 0) {
                    ret = -EIO;
                    goto out;
            }
            pos += ret;
    }
    ret = 0;
out:
    set_fs(old_fs);
    return ret;
}

static int kapi_vfs_file_sync(void* file)
{
    struct file* file_ = (struct file*)file;

    return vfs_fsync(file_, 0);
}

static void kapi_vfs_file_close(void* file)
{
    struct file* file_ = (struct file*)file;

    fput(file_);
}

struct kapi_misc_dev
{
    struct miscdevice misc;
    void* context;
    struct list_head list;
    atomic_t ref_count;
    long (*ioctl)(void* context, unsigned int code, unsigned long arg);
};

static LIST_HEAD(misc_devs);
static DEFINE_SPINLOCK(misc_devs_lock);

void kapi_misc_dev_get(struct kapi_misc_dev* dev)
{
    atomic_inc(&dev->ref_count);
}

void kapi_misc_dev_put(struct kapi_misc_dev* dev)
{
    if (atomic_dec_and_test(&dev->ref_count))
    {
        kapi_kfree(dev);
    }
}

struct kapi_misc_dev* kapi_misc_dev_find_by_context(void *context)
{
    struct kapi_misc_dev* curr;

    list_for_each_entry(curr, &misc_devs, list)
    {
        if (curr->context == context)
        {
            kapi_misc_dev_get(curr);
            return curr;
        }
    }

    return NULL;
}

struct kapi_misc_dev* kapi_misc_dev_find_by_dev(void* dev)
{
    struct kapi_misc_dev* dev_ = (struct kapi_misc_dev*)dev;
    struct kapi_misc_dev* curr;

    list_for_each_entry(curr, &misc_devs, list)
    {
        if (curr == dev_)
        {
            kapi_misc_dev_get(curr);
            return curr;
        }
    }

    return NULL;
}

static struct kapi_misc_dev* kapi_misc_dev_find_by_devt(dev_t devt)
{
    struct kapi_misc_dev* curr;

    list_for_each_entry(curr, &misc_devs, list)
    {
        if (curr->misc.this_device && curr->misc.this_device->devt == devt)
        {
            kapi_misc_dev_get(curr);
            return curr;
        }
    }

    return NULL;
}

static long kapi_ioctl(struct file* file, unsigned int code, unsigned long arg)
{
    struct kapi_misc_dev* dev;

    spin_lock(&misc_devs_lock);
    dev = kapi_misc_dev_find_by_devt(file->f_inode->i_rdev);
    spin_unlock(&misc_devs_lock);

    if (dev)
    {
        int rc = dev->ioctl(dev->context, code, arg);
        kapi_misc_dev_put(dev);
        return rc;
    }

    return -EFAULT;
}

static int kapi_module_get(struct inode *inode, struct file *file)
{
    if (!try_module_get(THIS_MODULE))
    {
            return -EINVAL;
    }
    return 0;
}

static int kapi_module_put(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);
    return 0;
}

static const struct file_operations kapi_fops =
{
    .owner = THIS_MODULE,
    .open = kapi_module_get,
    .release = kapi_module_put,
    .unlocked_ioctl = kapi_ioctl,
};

int kapi_misc_dev_register(const char* name, void* context,
    long (*ioctl)(void* context, unsigned int code, unsigned long arg),
    void** dev)
{
    struct kapi_misc_dev* dev_;
    struct kapi_misc_dev* existing_dev;
    int rc;
    int inserted;

    dev_ = kapi_kmalloc_gfp(sizeof(*dev_), GFP_KERNEL);
    if (!dev_)
    {
        PRINTK("No memory\n");
        return -ENOMEM;
    }

    memset(dev_, 0, sizeof(*dev_));
    dev_->misc.fops = &kapi_fops;
    dev_->misc.minor = MISC_DYNAMIC_MINOR;
    dev_->misc.name = name;
    dev_->context = context;
    dev_->ioctl = ioctl;
    atomic_set(&dev_->ref_count, 1);

    inserted = 0;
    spin_lock(&misc_devs_lock);
    existing_dev = kapi_misc_dev_find_by_context(context);
    if (!existing_dev)
    {
        kapi_misc_dev_get(dev_);
        list_add_tail(&dev_->list, &misc_devs);
        inserted = 1;
    }
    else
    {
        kapi_misc_dev_put(existing_dev);
    }
    spin_unlock(&misc_devs_lock);

    if (!inserted)
    {
        kapi_misc_dev_put(dev_);
        return -EFAULT;
    }

    rc = misc_register(&dev_->misc);
    if (rc)
    {
        PRINTK("Misc %s register failed %d\n", name, rc);
        spin_lock(&misc_devs_lock);
        list_del_init(&dev_->list);
        spin_unlock(&misc_devs_lock);
        kapi_misc_dev_put(dev_); //list
        kapi_misc_dev_put(dev_); //create
        return rc;
    }

    *dev = dev_;
    return 0;
}

void kapi_misc_dev_unregister(void* dev)
{
    struct kapi_misc_dev* dev_ = NULL;

    spin_lock(&misc_devs_lock);
    dev_ = kapi_misc_dev_find_by_dev(dev);
    if (dev_)
    {
        list_del_init(&dev_->list);
    }
    spin_unlock(&misc_devs_lock);

    if (dev_)
    {
        misc_deregister(&dev_->misc);
        kapi_misc_dev_put(dev_); // find
        kapi_misc_dev_put(dev_); // list
        kapi_misc_dev_put(dev_); // create
    }
}

static int kapi_copy_to_user(void* dst, const void* src, unsigned long size)
{
    if (copy_to_user(dst, src, size))
    {
        return -EFAULT;
    }

    return 0;
}

static int kapi_copy_from_user(void* dst, const void* src, unsigned long size)
{
    if (copy_from_user(dst, src, size))
    {
        return -EFAULT;
    }

    return 0;
}

static unsigned long long kapi_get_time(void)
{
    return ktime_get_ns();
}

static void kapi_trace_msg(const char* fmt, va_list args)
{
    trace_msg(fmt, args);
}

static void kapi_set_bit(long nr, unsigned long *addr)
{
    set_bit(nr, addr);
}

static void kapi_clear_bit(long nr, unsigned long *addr)
{
    clear_bit(nr, addr);
}

static int kapi_test_and_clear_bit(long nr, unsigned long *addr)
{
    return test_and_clear_bit(nr, addr);
}

static int kapi_test_and_set_bit(long nr, unsigned long *addr)
{
    return test_and_set_bit(nr, addr);
}

static int kapi_sock_connect(void **sockp, const char *host, unsigned short port)
{
    return ksock_connect_host((struct socket **)sockp, host, port);
}

static int kapi_sock_listen(void **sockp, const char *host, int port, int backlog)
{
    return ksock_listen_host((struct socket **)sockp, host, port, backlog);
}

static void kapi_sock_release(void *sockp)
{
    ksock_release((struct socket *)sockp);
}

static int kapi_sock_send(void *sockp, const void *buf, int len)
{
    return ksock_send((struct socket *)sockp, buf, len);
}

static int kapi_sock_recv(void *sockp, void *buf, int len)
{
    return ksock_recv((struct socket *)sockp, buf, len);
}

static int kapi_sock_accept(void **newsockp, void *sockp)
{
    return ksock_accept((struct socket **)newsockp, (struct socket *)sockp);
}

static void kapi_sock_abort_accept(void *sockp)
{
    return ksock_abort_accept((struct socket *)sockp);
}

static unsigned int kapi_le32_to_cpu(unsigned int value)
{
    return le32_to_cpu(value);
}

static unsigned int kapi_cpu_to_le32(unsigned int value)
{
    return cpu_to_le32(value);
}

static unsigned long long kapi_le64_to_cpu(unsigned long long value)
{
    return le64_to_cpu(value);
}

static unsigned long long kapi_cpu_to_le64(unsigned long long value)
{
    return cpu_to_le64(value);
}

static struct kernel_api g_kapi =
{
    .kmalloc = kapi_kmalloc,
    .kfree = kapi_kfree,

    .memset = kapi_memset,
    .memcmp = kapi_memcmp,
    .memcpy = kapi_memcpy,

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
    .completion_wait_timeout = kapi_completion_wait_timeout,
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
    .spinlock_lock_irqsave = kapi_spinlock_lock_irqsave,
    .spinlock_unlock_irqrestore = kapi_spinlock_unlock_irqrestore,

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
    .bdev_get_size = kapi_bdev_get_size,

    .alloc_bio = kapi_alloc_bio,
    .free_bio = kapi_free_bio,
    .set_bio_page = kapi_set_bio_page,
    .set_bio_end_io = kapi_set_bio_end_io,
    .set_bio_bdev = kapi_set_bio_bdev,
    .set_bio_flags = kapi_set_bio_flags,
    .set_bio_position = kapi_set_bio_position,
    .get_bio_private = kapi_get_bio_private,
    .submit_bio = kapi_submit_bio,

    .vfs_file_open = kapi_vfs_file_open,
    .vfs_file_read = kapi_vfs_file_read,
    .vfs_file_write = kapi_vfs_file_write,
    .vfs_file_sync = kapi_vfs_file_sync,
    .vfs_file_close = kapi_vfs_file_close,

    .misc_dev_register = kapi_misc_dev_register,
    .misc_dev_unregister = kapi_misc_dev_unregister,

    .copy_to_user = kapi_copy_to_user,
    .copy_from_user = kapi_copy_from_user,

    .get_time = kapi_get_time,

    .trace_msg = kapi_trace_msg,

    .set_bit = kapi_set_bit,
    .clear_bit = kapi_clear_bit,
    .test_and_set_bit = kapi_test_and_set_bit,
    .test_and_clear_bit = kapi_test_and_clear_bit,

    .sock_connect = kapi_sock_connect,
    .sock_listen = kapi_sock_listen,
    .sock_release = kapi_sock_release,
    .sock_send = kapi_sock_send,
    .sock_recv = kapi_sock_recv,
    .sock_accept = kapi_sock_accept,
    .sock_abort_accept = kapi_sock_abort_accept,

    .le32_to_cpu = kapi_le32_to_cpu,
    .cpu_to_le32 = kapi_cpu_to_le32,
    .le64_to_cpu = kapi_le64_to_cpu,
    .cpu_to_le64 = kapi_cpu_to_le64,

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
