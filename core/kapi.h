#pragma once

#include "../system/kapi.h"
#include "pool_type.h"

void kapi_init(struct kernel_api* api);

struct kernel_api *get_kapi(void);

static inline unsigned long get_kapi_pool_type(Core::Memory::PoolType poolType)
{
    switch (poolType)
    {
    case Core::Memory::PoolType::Atomic:
        return KAPI_POOL_TYPE_ATOMIC;
    case Core::Memory::PoolType::Kernel:
        return KAPI_POOL_TYPE_KERNEL;
    case Core::Memory::PoolType::User:
        return KAPI_POOL_TYPE_USER;
    case Core::Memory::PoolType::NoIO:
        return KAPI_POOL_TYPE_NOIO;
    case Core::Memory::PoolType::NoFS:
        return KAPI_POOL_TYPE_NOFS;
    default:
        return KAPI_POOL_TYPE_UNKNOWN;
    }
}
