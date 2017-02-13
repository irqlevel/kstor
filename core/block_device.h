#pragma once

#include "error.h"
#include "astring.h"
#include "page.h"

namespace Core
{

class BlockDevice
{
public:
    BlockDevice(const AString& deviceName, Error& err);
    void* GetBdev();
    unsigned long long GetSize() const;
    virtual ~BlockDevice();

    Core::Error Write(Page& page, unsigned long long position);
    Core::Error Read(Page& page, unsigned long long position);
    Core::Error ExecIo(Page& page, unsigned long long position, bool write);

private:
    void* BDevPtr;
    int Mode;
};

}