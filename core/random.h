#pragma once

#include "type.h"

namespace Core
{

class Random
{
public:
    static void GetBytes(void* buf, int len);
    static uint64_t GetUint64();
    static size_t GetSizeT();
    static size_t GetSizeT(size_t upper);
private:
    Random() = delete;
    Random(const Random& other) = delete;
    Random(Random&& other) = delete;
    Random& operator=(const Random& other) = delete;
    Random& operator=(Random&& other) = delete;

    static size_t Log2(size_t value);

};

}