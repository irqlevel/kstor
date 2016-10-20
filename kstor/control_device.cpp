#include "control_device.h"
#include "device.h"

#include <core/trace.h>
#include <core/copy_user.h>
#include <core/time.h>
#include <core/unique_ptr.h>
#include <core/trace.h>
#include <core/auto_lock.h>
#include <core/shared_auto_lock.h>

#include <include/ctl.h>

ControlDevice::ControlDevice(Error& err)
    : MiscDevice(KSTOR_CONTROL_DEVICE, err)
    , Rng(err)
{
}

Error ControlDevice::DeviceAdd(const char* deviceName, bool format, unsigned long& deviceId)
{
    Error err;

    DeviceRef device(new (Memory::PoolType::Kernel) Device(deviceName, format, err));
    if (device.Get() == nullptr)
    {
        trace(1, "CtrlDev 0x%p can't allocate device", this);
        err = Error::NoMemory;
        return err;
    }

    if (err != Error::Success)
    {
        trace(1, "CtrlDev 0x%p device init err %d", this, err.GetCode());
        return err;
    }

    AutoLock lock(DeviceListLock);
    if (!DeviceList.AddHead(device))
    {
        trace(1, "CtrlDev 0x%p can't add device into list");
        err = Error::NoMemory;
        return err;
    }

    deviceId = device->GetId();
    err = Error::Success;
    return err;
}

DeviceRef ControlDevice::DeviceLookup(unsigned long deviceId)
{
    SharedAutoLock lock(DeviceListLock);

    auto it = DeviceList.GetIterator();
    while (it.IsValid())
    {
        auto device = it.Get();
        if (device->GetId() == deviceId)
        {
            return device;
        }
        it.Next();
    }

    return DeviceRef();
}

DeviceRef ControlDevice::DeviceLookup(const AString& deviceName)
{
    SharedAutoLock lock(DeviceListLock);

    auto it = DeviceList.GetIterator();
    while (it.IsValid())
    {
        auto device = it.Get();
        if (device->GetName().Compare(deviceName) == 0)
        {
            return device;
        }
        it.Next();
    }

    return DeviceRef();
}

Error ControlDevice::DeviceRemove(unsigned long deviceId)
{
    DeviceRef device = DeviceLookup(deviceId);
    if (device.Get() == nullptr)
    {
        return Error::NotFound;
    }

    AutoLock lock(DeviceListLock);
    auto it = DeviceList.GetIterator();
    while (it.IsValid())
    {
        auto device = it.Get();
        if (device->GetId() == deviceId)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Error::Success;
}

Error ControlDevice::DeviceRemove(const AString& deviceName)
{
    DeviceRef device = DeviceLookup(deviceName);
    if (device.Get() == nullptr)
    {
        return Error::NotFound;
    }

    AutoLock lock(DeviceListLock);
    auto it = DeviceList.GetIterator();
    while (it.IsValid())
    {
        auto device = it.Get();
        if (device->GetName().Compare(deviceName) == 0)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Error::Success;
}

Error ControlDevice::Ioctl(unsigned int code, unsigned long arg)
{
    trace(1, "Ioctl 0x%x arg 0x%lx", code, arg);

    Error err;
    UniquePtr<KStorCtlCmd> cmd = UniquePtr<KStorCtlCmd>(new (Memory::PoolType::Kernel) KStorCtlCmd);
    if (cmd.Get() == nullptr)
    {
        trace(0, "Can't allocate memory");
        goto cleanup;
    }

    err = CopyFromUser(cmd.Get(), reinterpret_cast<KStorCtlCmd*>(arg));
    if (err != Error::Success)
    {
        trace(0, "Can't copy cmd from user");
        goto cleanup;
    }

    switch (code)
    {
    case IOCTL_KSTOR_GET_TIME:
        cmd->Union.GetTime.Time = Time::GetTime();
        break;
    case IOCTL_KSTOR_GET_RANDOM_ULONG:
        cmd->Union.GetRandomUlong.Value = Rng.GetUlong();
        break;
    case IOCTL_KSTOR_DEVICE_ADD:
        if (cmd->Union.DeviceAdd.DeviceName[Memory::ArraySize(cmd->Union.DeviceAdd.DeviceName) - 1] != '\0')
        {
            err = Error::InvalidValue;
            break;
        }

        err = DeviceAdd(cmd->Union.DeviceAdd.DeviceName, cmd->Union.DeviceAdd.Format,
            cmd->Union.DeviceAdd.DeviceId);
        break;
    case IOCTL_KSTOR_DEVICE_REMOVE:
        err = DeviceRemove(cmd->Union.DeviceRemove.DeviceId);
        break;
    case IOCTL_KSTOR_DEVICE_REMOVE_BY_NAME:
    {
        if (cmd->Union.DeviceRemoveByName.DeviceName[
                Memory::ArraySize(cmd->Union.DeviceRemoveByName.DeviceName) - 1] != '\0')
        {
            err = Error::InvalidValue;
            break;
        }

        AString deviceName(cmd->Union.DeviceRemoveByName.DeviceName, err);
        if (err != Error::Success)
        {
            break;
        }

        err = DeviceRemove(deviceName);
        break;
    }
    default:
        trace(0, "Unknown ioctl 0x%x", code);
        err = Error::UnknownCode;
        break;
    }

    if (err != Error::Success)
    {
        goto cleanup;
    }

    err = CopyToUser(reinterpret_cast<KStorCtlCmd*>(arg), cmd.Get());
    if (err != Error::Success)
    {
        trace(0, "Can't copy cmd to user");
        goto cleanup;
    }

cleanup:

    trace(1, "Ioctl 0x%x arg 0x%lx result %d", code, arg, err.GetCode());
    return err;
}


ControlDevice::~ControlDevice()
{
}