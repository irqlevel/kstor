#pragma once

#include "error.h"
#include "memory.h"

namespace Core
{

class BitmapInterface
{
public:
    virtual Error SetBit(size_t bit) = 0;
    virtual Error ClearBit(size_t bit) = 0;
    virtual Error TestAndSetBit(size_t bit, bool& oldValue) = 0;
    virtual Error TestAndClearBit(size_t bit, bool& oldValue) = 0;
    virtual Error FindSetZeroBit(size_t& bit) = 0;
};

class Bitmap : public BitmapInterface
{
public:
    Bitmap(void *buf, size_t size);
    virtual ~Bitmap();

    virtual Error SetBit(size_t bit) override;
    virtual Error ClearBit(size_t bit) override;
    virtual Error TestAndSetBit(size_t bit, bool& oldValue) override;
    virtual Error TestAndClearBit(size_t bit, bool& oldValue) override;
    virtual Error FindSetZeroBit(size_t& bit) override;

    void* GetBuf();

private:
    Error FindSetZeroBit(unsigned long* value, size_t maxBits, size_t& bit);

    Bitmap(const Bitmap& other) = delete;
    Bitmap(Bitmap&& other) = delete;
    Bitmap& operator=(const Bitmap& other) = delete;
    Bitmap& operator=(Bitmap&& other) = delete;

    void *Buf;
    size_t Size;
    size_t BitSize;
};

}