#pragma once

#include "../system/kapi.h"
#include "pool_type.h"

void kapi_init(struct kernel_api* api);

struct kernel_api *get_kapi(void);

static inline unsigned long get_kapi_pool_type(Memory::PoolType poolType)
{
    switch (poolType)
    {
    case Memory::PoolType::Atomic:
        return KAPI_POOL_TYPE_ATOMIC;
    case Memory::PoolType::Kernel:
        return KAPI_POOL_TYPE_KERNEL;
    case Memory::PoolType::User:
        return KAPI_POOL_TYPE_USER;
    case Memory::PoolType::NoIO:
        return KAPI_POOL_TYPE_NOIO;
    case Memory::PoolType::NoFS:
        return KAPI_POOL_TYPE_NOFS;
    default:
        return KAPI_POOL_TYPE_UNKNOWN;
    }
}
