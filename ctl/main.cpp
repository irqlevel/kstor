#include "ctl.h"

#include <stdio.h>
#include <string>

int main(int argc, char* argv[])
{
    int err = 0;
    KStorCtl ctl(err);
    if (err)
    {
        printf("Ctl open err %d\n", err);
        return err;
    }

    if (argc < 2) {
        printf("Invalid number of args\n");
        return 1;
    }

    std::string cmd(argv[1]);
    if (cmd == "mount")
    {
        if (argc != 3)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        std::string deviceName(argv[2]);
        unsigned long deviceId;
        err = ctl.Mount(deviceName.c_str(), true, deviceId);
        if (err)
        {
            printf("Ctl device add err %d\n", err);
            return err;
        }

        return 0;
    }
    else if (cmd == "umount")
    {
        if (argc != 3)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        std::string deviceName(argv[2]);
        err = ctl.Unmount(deviceName.c_str());
        if (err)
        {
            printf("Ctl device remove err %d\n", err);
            return err;
        }

        return 0;
    }
    else
    {
        printf("Unknown cmd %s\n", cmd.c_str());
        return 1;
    }

    return 0;
}