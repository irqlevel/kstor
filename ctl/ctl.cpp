#include "ctl.h"
#include "memory.h"

#include <include/ctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

KStorCtl::KStorCtl(int& err)
    : DevFd(-1)
{
    if (err)
    {
        return;
    }

    int fd = open("/dev/" KSTOR_CONTROL_DEVICE, 0);
    if (fd == -1)
    {
        err = errno;
        return;
    }
    DevFd = fd;
    return;
}


int KStorCtl::GetTime(unsigned long long& time)
{
    KStorCtlCmd cmd;

    time = 0;
    memset(&cmd, 0, sizeof(cmd));
    int err = ioctl(DevFd, IOCTL_KSTOR_GET_TIME, &cmd);
    if (!err)
    {
        time = cmd.Union.GetTime.Time;
    }

    return err;
}

int KStorCtl::GetRandomUlong(unsigned long& value)
{
    KStorCtlCmd cmd;

    value = -1;
    memset(&cmd, 0, sizeof(cmd));
    int err = ioctl(DevFd, IOCTL_KSTOR_GET_RANDOM_ULONG, &cmd);
    if (!err)
    {
        value = cmd.Union.GetRandomUlong.Value;
    }

    return err;
}

int KStorCtl::DeviceAdd(const char* deviceName, bool format, unsigned long& deviceId)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    snprintf(cmd.Union.DeviceAdd.DeviceName, ArraySize(cmd.Union.DeviceAdd.DeviceName), "%s", deviceName);
    cmd.Union.DeviceAdd.Format = format;

    int err = ioctl(DevFd, IOCTL_KSTOR_DEVICE_ADD, &cmd);
    if (!err)
    {
        deviceId = cmd.Union.DeviceAdd.DeviceId;
    }

    return err;
}

int KStorCtl::DeviceRemove(unsigned long& deviceId)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.Union.DeviceRemove.DeviceId = deviceId;

    return ioctl(DevFd, IOCTL_KSTOR_DEVICE_REMOVE, &cmd);
}

int KStorCtl::DeviceRemove(const char* deviceName)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    snprintf(cmd.Union.DeviceRemoveByName.DeviceName, ArraySize(cmd.Union.DeviceRemoveByName.DeviceName),
        "%s", deviceName);

    return ioctl(DevFd, IOCTL_KSTOR_DEVICE_REMOVE_BY_NAME, &cmd);
}

KStorCtl::~KStorCtl()
{
    if (DevFd >= 0)
    {
        close(DevFd);
        DevFd = -1;
    }
}