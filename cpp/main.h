#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "error.h"
#include "mem_type.h"

#include "../inc/consts.h"
#include "../core/kapi.h"
#include "new_delete.h"

struct kernel_api *get_kapi(void);

static inline unsigned long get_kapi_mem_flag(MemType memType)
{
    switch (memType)
    {
    case MemType::Atomic:
        return KAPI_MEM_ATOMIC;
    case MemType::Kernel:
        return KAPI_MEM_KERNEL;
    case MemType::User:
        return KAPI_MEM_USER;
    case MemType::NoIO:
        return KAPI_MEM_NOIO;
    case MemType::NoFS:
        return KAPI_MEM_NOFS;
    default:
        return KAPI_MEM_UNKNOWN;
    }
}

#define KBUG_ON(cond)   \
    get_kapi()->bug_on(cond)

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)

#ifdef __cplusplus
}
#endif
