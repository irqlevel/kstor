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
        } Mount;

        struct 
        {
            unsigned long DeviceId;
        } Unmount;

        struct
        {
            char DeviceName[DeviceNameMaxChars];
        } UnmountByName;

    } Union;
};

#pragma pack(pop)

#define IOCTL_KSTOR_GET_TIME                _IOWR(KSTOR_IOC_MAGIC, 1, KStorCtlCmd*)
#define IOCTL_KSTOR_GET_RANDOM_ULONG        _IOWR(KSTOR_IOC_MAGIC, 2, KStorCtlCmd*)

#define IOCTL_KSTOR_MOUNT             _IOWR(KSTOR_IOC_MAGIC, 3, KStorCtlCmd*)
#define IOCTL_KSTOR_UNMOUNT           _IOWR(KSTOR_IOC_MAGIC, 4, KStorCtlCmd*)
#define IOCTL_KSTOR_UNMOUNT_BY_NAME   _IOWR(KSTOR_IOC_MAGIC, 5, KStorCtlCmd*)