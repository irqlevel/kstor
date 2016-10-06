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

    for (int i = 0; i < 10; i++)
    {
        unsigned long long time;
        err = ctl.GetTime(time);
        if (err)
        {
            printf("Get time err %d\n", err);
            return err;
        }
        printf("Time is %llu\n", time);
    }

    return 0;
}