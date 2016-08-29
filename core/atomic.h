#pragma once

#include "kapi.h"

class Atomic
{
public:
    Atomic();
    Atomic(int value);
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
    struct kapi_atomic Value;
};
