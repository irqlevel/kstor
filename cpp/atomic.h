#pragma once

#include "main.h"

class Atomic
{
public:
    Atomic();
    Atomic(int value, int& err, MemType memType = MemType::Kernel);
    void Inc();
    bool DecAndTest();
    int Get();
    ~Atomic();
private:
    void *pAtomic;
};
