#pragma once

#include "main.h"

class Atomic
{
public:
    Atomic(int value, int& err);
    void Inc();
    bool DecAndTest();
    int Get();
    ~Atomic();
private:
    void *pAtomic;
};
