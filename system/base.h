#pragma once

#include <linux/kthread.h>

static inline unsigned long hash_pointer(void *ptr)
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

static inline char* truncate_file_name(char* file_name)
{
    char* base;

    base = strrchr(file_name, '/');
    if (base)
        return ++base;
    else
        return file_name;
}

#define PRINTK(fmt, ...)    \
    printk(__MODULE_NAME__ ": " fmt, ##__VA_ARGS__)
