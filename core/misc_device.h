#pragma once

#include "astring.h"
#include "error.h"

class MiscDevice
{
public:
    MiscDevice(const AString& devName, Error& err);
    virtual ~MiscDevice();

    virtual Error Ioctl(unsigned int code, unsigned long arg);

private:
    static long Ioctl(void* context, unsigned int code, unsigned long arg);

    void* MiscDevPtr;
    AString DevName;
};