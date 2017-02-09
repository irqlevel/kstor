#pragma once

namespace Core
{

#define OFFSET_OF(type, field)  \
            (unsigned long)&((type*)0)->field

#define CONTAINING_RECORD(addr, type, field)    \
            (type*)((unsigned long)(addr) - (unsigned long)&((type*)0)->field)


}