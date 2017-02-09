#include "misc_device.h"
#include "memory.h"
#include "kapi.h"
#include "trace.h"

namespace Core
{

MiscDevice::MiscDevice()
{
}

MiscDevice::MiscDevice(const AString& devName, Error& err)
{
    if (!err.Ok())
    {
        return;
    }

    err = Create(devName);
}

MiscDevice::MiscDevice(const char* devName, Error& err)
{
    if (!err.Ok())
    {
        return;
    }

    err = Create(devName);
}

Error MiscDevice::Create(const char* devName)
{
    Error err;

    AString devName_(devName, err);
    if (!err.Ok())
    {
        return err;
    }
    return Create(devName_);
}

Error MiscDevice::Create(const AString& devName)
{
    Error err;
    err = get_kapi()->misc_dev_register(devName.GetBuf(), this, &MiscDevice::Ioctl, &MiscDevPtr);
    if (!err.Ok())
    {
        trace(0, "Device %s register failed, err %d", devName.GetBuf(), err.GetCode());
        return err;
    }

    trace(4, "Device 0x%p dev 0x%p name %s", this, MiscDevPtr, devName.GetBuf());
    return err;
}

MiscDevice::~MiscDevice()
{
    trace(4, "Device 0x%p dev 0x%p dtor", this, MiscDevPtr);

    if (MiscDevPtr != nullptr)
    {
        get_kapi()->misc_dev_unregister(MiscDevPtr);
        MiscDevPtr = nullptr;
    }
}

long MiscDevice::Ioctl(void* context, unsigned int code, unsigned long arg)
{
    MiscDevice* device = static_cast<MiscDevice*>(context);

    Error err = device->Ioctl(code, arg);
    return err.GetCode();
}

Error MiscDevice::Ioctl(unsigned int code, unsigned long arg)
{
    return Error::NotImplemented;
}

}