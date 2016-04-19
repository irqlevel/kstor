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

    Atomic& operator=(Atomic&& other);
    Atomic(Atomic&& other);

private:
    Atomic(const Atomic& other) = delete;
    Atomic& operator=(const Atomic& other) = delete;
    void *pAtomic;
};
