#pragma once

#include "bug.h"
#include "format.h"

namespace Core
{

class Trace
{
public:
    static void Output(int level, const char *fmt, ...);
    static void SetLevel(int level);
    static int GetLevel();

private:
    static int Level;
};

}

#define trace(level, fmt, ...)                                                                  \
do {                                                                                            \
    if (level == -1)                                                                            \
    {                                                                                           \
        Core::Printk("%d: %s(),%s,%d: " fmt "\n",                                               \
        level, __FUNCTION__, Core::Format::TruncateFileName(__FILE__), __LINE__, ##__VA_ARGS__);\
    }                                                                                           \
    Core::Trace::Output(level, "%d: %s(),%s,%d: " fmt,                                          \
        level, __FUNCTION__, Core::Format::TruncateFileName(__FILE__), __LINE__, ##__VA_ARGS__);\
} while (false)
