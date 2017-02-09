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
    void Update(void* buf, size_t len);
    unsigned long long GetDigest();
    static unsigned long long GetDigest(void *buf, size_t len);
private:
    long long State[11];
};

}