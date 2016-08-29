#pragma once

#include "main.h"
#include "atomic.h"

class Smp
{
public:
    static int GetCpuId();
    static void PreemptDisable();
    static void PreemptEnable();
    static void LockOnlineCpus();
    static void UnlockOnlineCpus();
    static void CallFunctionOtherCpus(void (*function)(void *data),
                                      void *data, bool wait);
    static bool CallFunctionCurrCpuOnly(void (*function)(void *data),
                                        void *data);
    static void Pause();
private:
    struct BlockAndWait
    {
        Atomic* Counter;
        Atomic* Block;
    };
    static void CountCpus(void *data);
    static void BlockAndWait(void *data);
};
