#pragma once

#include "error.h"
#include "memory.h"

namespace Core
{

class XXHash
{
public:
    XXHash();
    virtual ~XXHash();
    void Reset();
    void Update(const void* buf, size_t len);

    void GetSum(unsigned char result[8]);

    static void Sum(const void *buf, size_t len, unsigned char result[8]);

private:
    long long State[11];
};

}