#include "ctl.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    int err = 0;
    KStorCtl ctl(err);
    if (err)
    {
        printf("Ctl open err %d\n", err);
        return err;
    }

    unsigned long deviceId;

    err = ctl.DeviceAdd("/dev/loop10", true, deviceId);
    if (err)
    {
        printf("Ctl device add err %d\n", err);
        return err;
    }

    printf("Ctl deviceId is 0x%lx\n", deviceId);

    err = ctl.DeviceRemove(deviceId);
    if (err)
    {
        printf("Ctl device remove err %d\n", err);
        return err;
    }

    return 0;
}