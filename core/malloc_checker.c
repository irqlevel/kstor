#include "malloc_checker.h"

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/stacktrace.h>

#include "base.h"

#define MALLOC_CHECKER_STACK_ENTRIES 10
#define MALLOC_CHECKER_NR_LISTS 9973
#define MALLOC_CHECKER_SIGN1 0xBEDABEDA
#define MALLOC_CHECKER_SIGN2 0xCBDACBDA

struct malloc_entry
{
    struct list_head link;
    gfp_t flags;
    void *ptr;
    size_t size;
    unsigned long crc32;
#ifdef __MALLOC_CHECKER_STACK_TRACE__
    struct stack_trace stack;
    unsigned long stack_entries[MALLOC_CHECKER_STACK_ENTRIES];
#endif
};

struct malloc_checker
{
    struct list_head entries_list[MALLOC_CHECKER_NR_LISTS];
    spinlock_t       entries_list_lock[MALLOC_CHECKER_NR_LISTS];
};

static struct malloc_checker g_malloc_checker;

int malloc_checker_init(void)
{
    struct malloc_checker *checker = &g_malloc_checker;
    unsigned long i;

    PRINTK("Malloc checker init\n");

    for (i = 0; i < ARRAY_SIZE(checker->entries_list); i++)
    {
        INIT_LIST_HEAD(&checker->entries_list[i]);
        spin_lock_init(&checker->entries_list_lock[i]);
    }

    return 0;
}

static unsigned long hash_ptr(void *ptr)
{
    unsigned long val = (unsigned long)ptr;
    unsigned long hash, i, c;

    hash = 5381;
    val = val >> 3;
    for (i = 0; i < sizeof(val); i++)
    {
        c = (unsigned char)val & 0xFF;
        hash = ((hash << 5) + hash) + c;
        val = val >> 8;
    }

    return hash;
}

void *malloc_checker_kmalloc(size_t size, gfp_t flags)
{
    struct malloc_checker *checker = &g_malloc_checker;
    struct malloc_entry *entry;
    unsigned long *psign1, *psign2;
    void *ptr;
    unsigned long i;
    unsigned long irq_flags;

    entry = kmalloc(sizeof(*entry), flags);
    if (!entry)
        return NULL;

    memset(entry, 0, sizeof(*entry));

    psign1 = kmalloc(size + 2*sizeof(unsigned long), flags);
    if (!psign1)
    {
        kfree(entry);
        return NULL;
    }

    ptr = (void *)((unsigned long)psign1 + sizeof(unsigned long));
    psign2 = (unsigned long *)((unsigned long)ptr + size);
    *psign1 = MALLOC_CHECKER_SIGN1;
    *psign2 = MALLOC_CHECKER_SIGN2;

    entry->ptr = ptr;
    entry->size = size;
    entry->flags = flags;
    entry->crc32 = 0;
    INIT_LIST_HEAD(&entry->link);

#ifdef __MALLOC_CHECKER_STACK_TRACE__
    entry->stack.nr_entries = 0;
    entry->stack.max_entries = ARRAY_SIZE(entry->stack_entries);
    entry->stack.entries = entry->stack_entries;
    entry->stack.skip = 2;
    save_stack_trace(&entry->stack);
#endif

    i = hash_ptr(ptr) % ARRAY_SIZE(checker->entries_list);
    spin_lock_irqsave(&checker->entries_list_lock[i], irq_flags);
    list_add(&entry->link, &checker->entries_list[i]);
    spin_unlock_irqrestore(&checker->entries_list_lock[i], irq_flags);

#ifdef __MALLOC_CHECKER_PRINTK__
    PRINTK("Alloc entry %p ptr %p\n", entry, entry->ptr);
#endif

    return ptr;
}

void check_and_release_entry(struct malloc_checker *checker,
                             struct malloc_entry *entry)
{
    unsigned long *psign1, *psign2;
    void *ptr = entry->ptr;

    psign1 = (unsigned long *)((unsigned long)ptr - sizeof(unsigned long));
    psign2 = (unsigned long *)((unsigned long)ptr + entry->size);

    BUG_ON(*psign1 != MALLOC_CHECKER_SIGN1);
    BUG_ON(*psign2 != MALLOC_CHECKER_SIGN2);

    memset(entry->ptr, 0xCC, entry->size);

#ifdef __MALLOC_CHECKER_PRINTK__
    PRINTK("Free entry %p ptr %p\n", entry, entry->ptr);
#endif

    kfree(psign1);
    kfree(entry);
}

void malloc_checker_kfree(void *ptr)
{
    struct malloc_checker *checker = &g_malloc_checker;
    unsigned long i;
    unsigned long irq_flags;
    struct malloc_entry *curr, *tmp;
    struct list_head entries_list;

    INIT_LIST_HEAD(&entries_list);
    i = hash_ptr(ptr) % ARRAY_SIZE(checker->entries_list);
    spin_lock_irqsave(&checker->entries_list_lock[i], irq_flags);
    list_for_each_entry_safe(curr, tmp, &checker->entries_list[i], link)
    {
        if (curr->ptr == ptr)
        {
            list_del(&curr->link);
            list_add(&curr->link, &entries_list);
        }
    }
    spin_unlock_irqrestore(&checker->entries_list_lock[i], irq_flags);

    list_for_each_entry_safe(curr, tmp, &entries_list, link)
    {
        list_del_init(&curr->link);
        check_and_release_entry(checker, curr);
    }
}

void malloc_checker_deinit(void)
{
    unsigned long i;
    unsigned long irq_flags;
    struct list_head entries_list;
    struct malloc_entry *curr, *tmp;
    struct malloc_checker *checker = &g_malloc_checker;

    PRINTK("Malloc checker deinit\n");

    for (i = 0; i < ARRAY_SIZE(checker->entries_list); i++)
    {
        INIT_LIST_HEAD(&entries_list);
        spin_lock_irqsave(&checker->entries_list_lock[i], irq_flags);
        list_for_each_entry_safe(curr, tmp, &checker->entries_list[i], link)
        {
            list_del(&curr->link);
            list_add(&curr->link, &entries_list);
        }
        spin_unlock_irqrestore(&checker->entries_list_lock[i], irq_flags);

        list_for_each_entry_safe(curr, tmp, &entries_list, link)
        {
            list_del_init(&curr->link);
            PRINTK("Leak entry %p ptr %p size %lu flags 0x%x\n",
                   curr, curr->ptr, curr->size, curr->flags);
#ifdef __MALLOC_CHECKER_STACK_TRACE__
            {
                char stack[512];

                snprint_stack_trace(stack, sizeof(stack), &curr->stack, 0);
                stack[ARRAY_SIZE(stack)-1] = '\0';
                PRINTK("Stack %s\n", stack);
            }
#endif
            check_and_release_entry(checker, curr);
        }
    }
}
