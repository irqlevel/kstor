#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>

class Device
{
public:
    Device(const char* deviceName, bool format, Error& err);
    virtual ~Device();

    Error Format();
    Error Load();

    unsigned long GetId() const;
    unsigned long long GetSize() const;

    const AString& GetName() const;

private:
    AString DeviceName;
    BlockDevice BDev;
};

typedef SharedPtr<Device, Memory::PoolType::Kernel> DeviceRef;