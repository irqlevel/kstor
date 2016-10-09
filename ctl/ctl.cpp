#include "ctl.h"

#include <include/ctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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

KStorCtl::~KStorCtl()
{
    if (DevFd >= 0)
    {
        close(DevFd);
        DevFd = -1;
    }
}