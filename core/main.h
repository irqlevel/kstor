#pragma once

#include "error.h"
#include "memory.h"

#include "../inc/consts.h"
#include "../system/kapi.h"

#include "new_delete.h"
#include "utility.h"
#include "trace.h"

struct kernel_api *get_kapi(void);

static inline unsigned long get_kapi_gfp_flags(Memory::PoolType poolType)
{
    switch (poolType)
    {
    case Memory::PoolType::Atomic:
        return KAPI_GFP_ATOMIC;
    case Memory::PoolType::Kernel:
        return KAPI_GFP_KERNEL;
    case Memory::PoolType::User:
        return KAPI_GFP_USER;
    case Memory::PoolType::NoIO:
        return KAPI_GFP_NOIO;
    case Memory::PoolType::NoFS:
        return KAPI_GFP_NOFS;
    default:
        return KAPI_GFP_UNKNOWN;
    }
}

#define KBUG_ON(cond)   \
    get_kapi()->bug_on(cond)

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)
