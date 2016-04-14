#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>

#include "error.h"
#include "../inc/consts.h"
#include "../core/kapi.h"

struct kernel_api *get_kapi(void);

#define KBUG_ON(cond)   \
    get_kapi()->bug_on(cond)

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)

#ifdef __cplusplus
}
#endif
