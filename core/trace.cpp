#include "trace.h"
#include "kapi.h"

#include <stdarg.h>
#include <stdio.h>

namespace Core
{

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
    if (level > Level)
        return;

    va_list args;
    va_start(args, fmt);
    get_kapi()->trace_msg(fmt, args);
    va_end(args);

}

}
