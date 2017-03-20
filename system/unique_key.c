#include "unique_key.h"

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#include "base.h"
#include "kapi_internal.h"

struct unique_key_entry
{
    struct list_head list;
    void *key;
    void *value;
};

#define UNIQUE_KEY_NR_LISTS 10151

struct unique_key_table {
	struct list_head list[UNIQUE_KEY_NR_LISTS];
	spinlock_t	 lock[UNIQUE_KEY_NR_LISTS];
};

static struct unique_key_table g_unique_key_table;

static inline struct unique_key_table *unique_table_get(void)
{
    return &g_unique_key_table;
}

void unique_key_init(void)
{
    struct unique_key_table *table = unique_table_get();
    int i;

    for (i = 0; i < ARRAY_SIZE(table->list); i++)
    {
        INIT_LIST_HEAD(&table->list[i]);
        spin_lock_init(&table->lock[i]);
    }
}

int unique_key_register(void *key, void *value, gfp_t flags)
{
    struct unique_key_table *table = unique_table_get();
    struct unique_key_entry *new_entry, *entry, *exist;
    unsigned long irq_flags;
    int i;

    new_entry = kapi_kmalloc_gfp(sizeof(*new_entry), flags);
    if (!new_entry)
        return false;

    new_entry->key = key;
    new_entry->value = value;

    exist = NULL;
    i = hash_pointer(key) % ARRAY_SIZE(table->list);
    spin_lock_irqsave(&table->lock[i], irq_flags);

    list_for_each_entry(entry, &table->list[i], list) {
        if (entry->key == key) {
            exist = entry;
            break;
        }
    }

    if (!exist) {
        list_add_tail(&new_entry->list, &table->list[i]);
    }

    spin_unlock_irqrestore(&table->lock[i], irq_flags);

    if (exist) {
        kapi_kfree(new_entry);
        return -EEXIST;
    }

    return 0;
}

int unique_key_unregister(void *key, void *value)
{
    struct unique_key_table *table = unique_table_get();
    struct unique_key_entry *entry, *tmp, *exist;
    unsigned long irq_flags;
    int i;

    exist = NULL;

    i = hash_pointer(key) % ARRAY_SIZE(table->list);
    spin_lock_irqsave(&table->lock[i], irq_flags);

    list_for_each_entry_safe(entry, tmp, &table->list[i], list) {
        if (entry->key == key && entry->value == value) {
            list_del_init(&entry->list);
            exist = entry;
            break;
        }
    }

    spin_unlock_irqrestore(&table->lock[i], irq_flags);

    if (!exist)
        return -ENOTTY;

    kapi_kfree(exist);
    return 0;
}

void unique_key_deinit(void)
{
    struct unique_key_table *table = unique_table_get();
    struct list_head list;
    struct unique_key_entry *entry, *tmp;
    unsigned long irq_flags;
    int i;

    for (i = 0; i < ARRAY_SIZE(table->list); i++) {
        INIT_LIST_HEAD(&list);

        spin_lock_irqsave(&table->lock[i], irq_flags);
        list_splice_init(&table->list[i], &list);
        spin_unlock_irqrestore(&table->lock[i], irq_flags);

        list_for_each_entry_safe(entry, tmp, &list, list) {
            list_del_init(&entry->list);
            kapi_kfree(entry);
        }
    }
}