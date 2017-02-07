#include "volume.h"

#include <core/trace.h>
#include <core/bio.h>

namespace KStor
{

Volume::Volume(const Core::AString& deviceName, bool format, Core::Error& err)
    : DeviceName(deviceName, err)
    , Device(DeviceName, err)
    , HeaderPage(Core::Memory::PoolType::Kernel, err)
{
    if (!err.Ok())
    {
        goto out;
    }

    trace(1, "Device 0x%p size 0x%llx", this, Device.GetSize());

    HeaderPage.Zero();

    if (format)
    {
        err = Format();
    }

    if (!err.Ok())
    {
        goto out;
    }

    err = Load();

out:
    trace(1, "Device 0x%p name %s ctor err %d", this, deviceName.GetBuf(), err.GetCode());
}

Volume::~Volume()
{
    trace(1, "Device 0x%p dtor", this);
}

Core::Error Volume::Format()
{
    Core::Error err;

    Core::Bio bio(Device, HeaderPage, 0, err, true);
    if (!err.Ok())
    {
        trace(0, "Can't init bio, err %d", err.GetCode());
        return err;
    }
    bio.Submit();
    bio.Wait();
    err = bio.GetError();

    trace(1, "Device 0x%p format err %d", this, err.GetCode());

    return err;
}

Core::Error Volume::Load()
{
    Core::Error err;

    Core::Bio bio(Device, HeaderPage, 0, err, false);
    if (!err.Ok())
    {
        trace(0, "Can't init bio, err %d", err.GetCode());
        return err;
    }
    bio.Submit();
    bio.Wait();
    err = bio.GetError();

    trace(1, "Device 0x%p load err %d", this, err.GetCode());

    return err;
}

const Guid& Volume::GetVolumeId() const
{
    return VolumeId;
}

unsigned long long Volume::GetSize() const
{
    return Device.GetSize();
}

const Core::AString& Volume::GetDeviceName() const
{
    return DeviceName;
}

Core::BlockDevice& Volume::GetDevice()
{
    return Device;
}

}