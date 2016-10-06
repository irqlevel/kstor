#pragma once

#include "ioctl.h"

#define KSTOR_CONTROL_DEVICE "kstor-control"

#define KSTOR_IOC_MAGIC 0xE1

#pragma pack(push, 1)

struct KStorCtlCmd
{
    union
    {
        struct
        {
            unsigned long long Time;
        } GetTime;
    } Union;
};

#pragma pack(pop)

#define IOCTL_KSTOR_GET_TIME      _IOWR(KSTOR_IOC_MAGIC, 1, KStorCtlCmd*)