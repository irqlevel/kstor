#pragma once

#include "bug.h"
#include "format.h"
#include "type.h"

namespace Core
{

class Trace
{
public:
    static void Output(int level, const char *fmt, ...);

    static inline void SetLevel(int level)
    {
        Level = level;
    }

    static inline int GetLevel()
    {
        return Level;
    }

private:
    static int Level;
};

}

#define trace(level, fmt, ...)                                                                      \
do {                                                                                                \
    if (unlikely(level == -1))                                                                      \
    {                                                                                               \
        Core::Printk("%d: %s(),%s,%d: " fmt "\n",                                                   \
        level, __FUNCTION__, Core::Format::TruncateFileName(__FILE__), __LINE__, ##__VA_ARGS__);    \
    }                                                                                               \
    if (unlikely(level <= Core::Trace::GetLevel()))                                                 \
    {                                                                                               \
        Core::Trace::Output(level, "%d: %s(),%s,%d: " fmt,                                          \
            level, __FUNCTION__, Core::Format::TruncateFileName(__FILE__), __LINE__, ##__VA_ARGS__);\
    }                                                                                               \
} while (false)
