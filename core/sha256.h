#pragma once

#include "error.h"
#include "memory.h"

namespace Core
{

class Sha256
{
public:
    Sha256();
    virtual ~Sha256();

    void Update(const void *buf, unsigned int len);
    void Finish(unsigned char output[32]);

private:
    void Process(const unsigned char data[64]);
    void Clear();

    unsigned int Total[2];
    unsigned int State[8];
    unsigned char Buffer[64];

    unsigned char Ipad[64];
    unsigned char Opad[64];
};

}