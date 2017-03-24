#include "trace.h"
#include "bug.h"
#include "kapi.h"

#include <stdarg.h>
#include <stdio.h>

namespace Core
{

int Trace::Level = 1;

void Trace::Output(int level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    get_kapi()->trace_msg(fmt, args);
    va_end(args);
}

}
