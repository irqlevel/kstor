#include "device.h"

#include <core/trace.h>

Device::Device(const char* deviceName, bool format, Error& err)
    : DeviceName(deviceName, err)
    , BDev(DeviceName, err)
{
    if (err != Error::Success)
    {
        goto out;
    }

    if (format)
    {
        err = Format();
    }

    if (err != Error::Success)
    {
        goto out;
    }

    err = Load();

out:
    trace(1, "Device 0x%p name %s ctor err %d", this, deviceName, err.GetCode());
}

Error Device::Format()
{
    Error err;

    trace(1, "Device 0x%p format err %d", this, err.GetCode());

    return err;
}

Error Device::Load()
{
    Error err;

    trace(1, "Device 0x%p load err %d", this, err.GetCode());

    return err;
}

unsigned long Device::GetDeviceId()
{
    return reinterpret_cast<unsigned long>(this);
}

Device::~Device()
{
    trace(1, "Device 0x%p dtor", this);
}