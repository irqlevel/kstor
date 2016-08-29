#pragma once

#include "kapi.h"

#define BUG_ON(cond)   \
    get_kapi()->bug_on(cond)
