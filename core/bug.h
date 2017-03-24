#pragma once

#include "kapi.h"
#include "format.h"
#include "type.h"

namespace Core
{
    static inline void VPrintk(const char *fmt, va_list args)
    {
        get_kapi()->vprintk(fmt, args);
    }

    static inline void Printk(const char *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        VPrintk(fmt, args);
        va_end(args);
    }

    static inline bool Panic(bool condition, const char *prefix, const char *func, const char *file, int line)
    {
        if (condition)
        {
            Printk("%s: panic at %s(),%s,%d\n", prefix, func, file, line);

#if defined(__DEBUG__)
            get_kapi()->bug_on(condition);
#endif

            return true;
        }
        return false;
    }

}

#define panic(condition)    \
    (unlikely(condition)) ? \
        Core::Panic((condition) ? true : false, __MODULE_NAME__, __FUNCTION__, Core::Format::TruncateFileName(__FILE__), __LINE__) : \
        false
