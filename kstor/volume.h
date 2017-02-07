#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>
#include <core/page.h>

#include "guid.h"

namespace KStor 
{

class Volume
{
public:
    Volume(const Core::AString& deviceName, bool format, Core::Error& err);
    virtual ~Volume();

    Core::Error Format();
    Core::Error Load();

    const Guid& GetVolumeId() const;

    unsigned long long GetSize() const;

    const Core::AString& GetDeviceName() const;

    Core::BlockDevice& GetDevice();

private:
    Core::AString DeviceName;
    Core::BlockDevice Device;
    Core::Page HeaderPage;
    Guid VolumeId;
};

typedef Core::SharedPtr<Volume, Core::Memory::PoolType::Kernel> VolumePtr;

}