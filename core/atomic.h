#pragma once

#include "main.h"
#include "error.h"

class Atomic
{
public:
    Atomic();
    Atomic(int value, Error& err,
           Memory::PoolType poolType = Memory::PoolType::Kernel);
    void Inc();
    bool DecAndTest();
    int Get();
    void Set(int value);
    ~Atomic();

    Atomic& operator=(Atomic&& other);
    Atomic(Atomic&& other);

private:
    Atomic(const Atomic& other) = delete;
    Atomic& operator=(const Atomic& other) = delete;
    void *pAtomic;
};
