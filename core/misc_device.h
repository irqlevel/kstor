#pragma once

#include "astring.h"
#include "error.h"

namespace Core
{

class MiscDevice
{
public:
    MiscDevice();
    MiscDevice(const AString& devName, Error& err);
    MiscDevice(const char* devName, Error& err);

    virtual ~MiscDevice();

    virtual Error Ioctl(unsigned int code, unsigned long arg);

private:
    Error Create(const char* devName);
    Error Create(const AString& devName);

    static long Ioctl(void* context, unsigned int code, unsigned long arg);

    void* MiscDevPtr;
};

}