#pragma once

#include "kapi.h"

namespace Core
{
    static inline void BugOn(bool condition)
    {
        get_kapi()->bug_on(condition);
    }
}