#pragma once

#include "main.h"

class Trace
{
public:
    static void Output(int level, const char *prefix, const char *file,
                      const char *func, int line, int pid,
                      const char *fmt, ...);
    static void SetLevel(int level);
    static int GetLevel();
private:
    static int Level;
};

#define trace(level, fmt, ...)   \
            Trace::Output(level, KCPP,  \
                          __FILE__, __PRETTY_FUNCTION__, __LINE__,  \
                          get_kapi()->task_get_id(get_kapi()->task_current()), \
                          fmt, ##__VA_ARGS__)
