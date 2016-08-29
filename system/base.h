#pragma once

#include <linux/kthread.h>

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
    printk(__MODULE_NAME__ ": p%d %s,%d %s():" fmt, current->pid, \
           truncate_file_name(__FILE__),__LINE__,\
           __FUNCTION__, ##__VA_ARGS__)
