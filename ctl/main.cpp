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

    unsigned long long time;
    err = ctl.GetTime(time);
    if (err)
    {
        printf("GetTime err %d\n", err);
        return err;
    }
    printf("Time is %llu\n", time);

    for (int i = 0; i < 10; i++)
    {
        unsigned long value;
        err = ctl.GetRandomUlong(value);
        if (err)
        {
            printf("GetRandomUlong err %d\n", err);
            return err;
        }
        printf("Random is %lu\n", value);
    }

    return 0;
}