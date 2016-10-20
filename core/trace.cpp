#include "trace.h"
#include "kapi.h"

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

void Trace::Output(int level, const char *fmt, ...)
{
    char output[256];
    va_list args;

    if (level > Level)
        return;

    va_start(args, fmt);
    vsnprintf(output, sizeof(output), fmt, args);
    va_end(args);

    output[sizeof(output)/sizeof(char) - 1] = '\0';

    get_kapi()->trace_println(output);
}
