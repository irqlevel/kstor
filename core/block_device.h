#pragma once

#include "error.h"
#include "astring.h"

class BlockDevice
{
public:
    BlockDevice(const AString& deviceName, Error& err);
    void* GetBdev();
    unsigned long long GetSize() const;
    virtual ~BlockDevice();
private:
    void* BDevPtr;
    int Mode;
};