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

    auto& params = cmd.Union.Mount;
    snprintf(params.DeviceName, ArraySize(params.DeviceName), "%s", deviceName);
    params.Format = format;

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
    auto& params = cmd.Union.Unmount;
    params.DeviceId = deviceId;

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT, &cmd);
}

int KStorCtl::Unmount(const char* deviceName)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.UnmountByName;
    snprintf(params.DeviceName, ArraySize(params.DeviceName),
        "%s", deviceName);

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT_BY_NAME, &cmd);
}

int KStorCtl::StartServer(const char *host, unsigned short port)
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.StartServer;
    snprintf(params.Host, ArraySize(params.Host),
        "%s", host);
    params.Port = port;
    return ioctl(DevFd, IOCTL_KSTOR_START_SERVER, &cmd);
}

int KStorCtl::StopServer()
{
    KStorCtlCmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    return ioctl(DevFd, IOCTL_KSTOR_STOP_SERVER, &cmd);
}

KStorCtl::~KStorCtl()
{
    if (DevFd >= 0)
    {
        close(DevFd);
        DevFd = -1;
    }
}