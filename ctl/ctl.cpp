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

int KStorCtl::Mount(const char* deviceName, bool format, unsigned long& deviceId)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    snprintf(cmd.Union.Mount.DeviceName, ArraySize(cmd.Union.Mount.DeviceName), "%s", deviceName);
    cmd.Union.Mount.Format = format;

    int err = ioctl(DevFd, IOCTL_KSTOR_MOUNT, &cmd);
    if (!err)
    {
        deviceId = cmd.Union.Mount.DeviceId;
    }

    return err;
}

int KStorCtl::Unmount(unsigned long& deviceId)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.Union.Unmount.DeviceId = deviceId;

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT, &cmd);
}

int KStorCtl::Unmount(const char* deviceName)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    snprintf(cmd.Union.UnmountByName.DeviceName, ArraySize(cmd.Union.UnmountByName.DeviceName),
        "%s", deviceName);

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT_BY_NAME, &cmd);
}

KStorCtl::~KStorCtl()
{
    if (DevFd >= 0)
    {
        close(DevFd);
        DevFd = -1;
    }
}