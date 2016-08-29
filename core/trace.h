#pragma once

#include "kapi.h"

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
              Trace::Output(level, __MODULE_NAME__,  \
              __FILE__, __FUNCTION__, __LINE__,  \
              get_kapi()->task_get_id(get_kapi()->task_current()), \
              fmt, ##__VA_ARGS__)
