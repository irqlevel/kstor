#pragma once

#include "error.h"
#include "memory.h"

#include "../inc/consts.h"
#include "../system/kapi.h"

#include "new_delete.h"
#include "utility.h"
#include "trace.h"
#include "kapi.h"

#define BUG_ON(cond)   \
    get_kapi()->bug_on(cond)

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)
