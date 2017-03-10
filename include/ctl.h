#pragma once

#include "ioctl.h"
#include <kstor/api.h>

namespace KStor
{

namespace Control
{

#define KSTOR_CONTROL_DEVICE "kstor-control"

#define KSTOR_IOC_MAGIC 0xE1

const int DeviceNameMaxChars = 256;
const int HostNameMaxChars = 256;

#pragma pack(push, 1)

struct Cmd
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
            Api::Guid VolumeId;
            bool Format;
        } Mount;

        struct 
        {
            Api::Guid VolumeId;
        } Unmount;

        struct
        {
            char DeviceName[DeviceNameMaxChars];
        } UnmountByName;

        struct {
            char Host[HostNameMaxChars];
            unsigned short Port;
        } StartServer;

        struct {
            unsigned long Padding;
        } StopServer;

        struct {
            unsigned int TestId;
        } Test;

    } Union;
};

#pragma pack(pop)

}
}

#define IOCTL_KSTOR_GET_TIME                _IOWR(KSTOR_IOC_MAGIC, 1, KStor::Control::Cmd*)
#define IOCTL_KSTOR_GET_RANDOM_ULONG        _IOWR(KSTOR_IOC_MAGIC, 2, KStor::Control::Cmd*)

#define IOCTL_KSTOR_MOUNT             _IOWR(KSTOR_IOC_MAGIC, 3, KStor::Control::Cmd*)
#define IOCTL_KSTOR_UNMOUNT           _IOWR(KSTOR_IOC_MAGIC, 4, KStor::Control::Cmd*)
#define IOCTL_KSTOR_UNMOUNT_BY_NAME   _IOWR(KSTOR_IOC_MAGIC, 5, KStor::Control::Cmd*)

#define IOCTL_KSTOR_START_SERVER   _IOWR(KSTOR_IOC_MAGIC, 6, KStor::Control::Cmd*)
#define IOCTL_KSTOR_STOP_SERVER   _IOWR(KSTOR_IOC_MAGIC, 7, KStor::Control::Cmd*)

#define IOCTL_KSTOR_TEST    _IOWR(KSTOR_IOC_MAGIC, 8, KStor::Control::Cmd*)