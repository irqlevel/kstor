#pragma once

#include "kapi.h"

class Time
{
public:

    static unsigned long long GetTime()
    {
        return get_kapi()->get_time();
    }
};