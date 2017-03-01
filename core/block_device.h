#pragma once

#include "error.h"
#include "astring.h"
#include "page.h"
#include "bio.h"
#include "block_device_interface.h"

namespace Core
{

class BlockDevice : public BlockDeviceInterface
{
public:
    BlockDevice(const AString& deviceName, Error& err);
    virtual void* GetBdev() override;
    unsigned long long GetSize() const;
    virtual ~BlockDevice();

private:
    void* BDevPtr;
    int Mode;
};

}