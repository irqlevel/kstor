#include "misc_device.h"
#include "memory.h"
#include "kapi.h"
#include "trace.h"

MiscDevice::MiscDevice(const AString& devName, Error& err)
    : DevName(devName, Memory::PoolType::Kernel, err)
{
    if (err != Error::Success)
    {
        return;
    }

    err = get_kapi()->misc_dev_register(DevName.GetBuf(), this, &MiscDevice::Ioctl, &MiscDevPtr);
    if (err != Error::Success)
    {
        trace(0, "Misc device register failed, err %d", err.GetCode());
        err = Error::Unsuccessful;
        return;
    }

    trace(1, "Misc device 0x%p dev 0x%p constructed", this, MiscDevPtr);
}

MiscDevice::~MiscDevice()
{
    trace(1, "Misc device 0x%p dev 0x%p destructor", this, MiscDevPtr);

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