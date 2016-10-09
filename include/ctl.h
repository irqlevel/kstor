#pragma once

#include "ioctl.h"

#define KSTOR_CONTROL_DEVICE "kstor-control"

#define KSTOR_IOC_MAGIC 0xE1

#pragma pack(push, 1)

const int DeviceNameMaxChars = 256;

struct KStorCtlCmd
{
    union
    {
        struct
        {
            unsigned long long Time;
        } GetTime;

        struct
        {
            unsigned long Value;
        } GetRandomUlong;

        struct
        {
            char DeviceName[DeviceNameMaxChars];
            unsigned long DeviceId;
            bool Format;
        } DeviceAdd;

        struct 
        {
            unsigned long DeviceId;
        } DeviceRemove;

    } Union;
};

#pragma pack(pop)

#define IOCTL_KSTOR_GET_TIME            _IOWR(KSTOR_IOC_MAGIC, 1, KStorCtlCmd*)
#define IOCTL_KSTOR_GET_RANDOM_ULONG    _IOWR(KSTOR_IOC_MAGIC, 2, KStorCtlCmd*)

#define IOCTL_KSTOR_DEVICE_ADD          _IOWR(KSTOR_IOC_MAGIC, 3, KStorCtlCmd*)
#define IOCTL_KSTOR_DEVICE_REMOVE       _IOWR(KSTOR_IOC_MAGIC, 4, KStorCtlCmd*)