#include "trace.h"

#include <stdarg.h>
#include <stdio.h>

int Trace::Level = 1;

void Trace::SetLevel(int level)
{
    Level = level;
}

int Trace::GetLevel()
{
    return Level;
}

void Trace::Output(int level, const char *prefix, const char *file,
                      const char *func, int line, int pid,
                      const char *fmt, ...)
{
    char output[256];
    int pos;
    va_list args;

    if (level > Level)
        return;

    pos = snprintf(output, sizeof(output), "%s: p%d %s,%d %s:",
                   prefix, pid, file, line, func);
    if (pos < 0)
        return;

    va_start(args, fmt);
    vsnprintf(output + pos, sizeof(output) - pos,
              fmt, args);
    va_end(args);

    output[sizeof(output)-1] = '\0';

    get_kapi()->printk("%s\n", output);
}
