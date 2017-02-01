#include "control_device.h"
#include "super_block.h"

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

Error ControlDevice::Mount(const AString& deviceName, bool format, unsigned long& deviceId)
{
    Error err;

    SuperBlockRef super(new (Memory::PoolType::Kernel) SuperBlock(deviceName, format, err));
    if (super.Get() == nullptr)
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

    AutoLock lock(SuperBlockListLock);
    if (!SuperBlockList.AddHead(super))
    {
        trace(1, "CtrlDev 0x%p can't add device into list");
        err = Error::NoMemory;
        return err;
    }

    deviceId = super->GetId();
    err = Error::Success;
    return err;
}

SuperBlockRef ControlDevice::LookupMount(unsigned long deviceId)
{
    SharedAutoLock lock(SuperBlockListLock);

    auto it = SuperBlockList.GetIterator();
    while (it.IsValid())
    {
        auto super = it.Get();
        if (super->GetId() == deviceId)
        {
            return super;
        }
        it.Next();
    }

    return SuperBlockRef();
}

SuperBlockRef ControlDevice::LookupMount(const AString& deviceName)
{
    SharedAutoLock lock(SuperBlockListLock);

    auto it = SuperBlockList.GetIterator();
    while (it.IsValid())
    {
        auto super = it.Get();
        if (super->GetName().Compare(deviceName) == 0)
        {
            return super;
        }
        it.Next();
    }

    return SuperBlockRef();
}

Error ControlDevice::Unmount(unsigned long deviceId)
{
    SuperBlockRef super = LookupMount(deviceId);
    if (super.Get() == nullptr)
    {
        return Error::NotFound;
    }

    AutoLock lock(SuperBlockListLock);
    auto it = SuperBlockList.GetIterator();
    while (it.IsValid())
    {
        auto super = it.Get();
        if (super->GetId() == deviceId)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Error::Success;
}

Error ControlDevice::Unmount(const AString& deviceName)
{
    SuperBlockRef super = LookupMount(deviceName);
    if (super.Get() == nullptr)
    {
        return Error::NotFound;
    }

    AutoLock lock(SuperBlockListLock);
    auto it = SuperBlockList.GetIterator();
    while (it.IsValid())
    {
        auto super = it.Get();
        if (super->GetName().Compare(deviceName) == 0)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Error::Success;
}

Error ControlDevice::StartServer(const AString& host, unsigned short port)
{
    return Srv.Start(host, port);
}

Error ControlDevice::StopServer()
{
    Srv.Stop();
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
    case IOCTL_KSTOR_MOUNT:
    {
        auto& params = cmd->Union.Mount;
        if (params.DeviceName[Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err = Error::InvalidValue;
            break;
        }

        AString deviceName(params.DeviceName, Memory::ArraySize(params.DeviceName) - 1, err);
        if (err != Error::Success)
        {
            break;
        }

        err = Mount(deviceName, params.Format, params.DeviceId);
        break;
    }
    case IOCTL_KSTOR_UNMOUNT:
        err = Unmount(cmd->Union.Unmount.DeviceId);
        break;
    case IOCTL_KSTOR_UNMOUNT_BY_NAME:
    {
        auto& params = cmd->Union.UnmountByName;
        if (params.DeviceName[Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err = Error::InvalidValue;
            break;
        }

        AString deviceName(params.DeviceName, Memory::ArraySize(params.DeviceName) - 1, err);
        if (err != Error::Success)
        {
            break;
        }

        err = Unmount(deviceName);
        break;
    }
    case IOCTL_KSTOR_START_SERVER:
    {
        auto& params = cmd->Union.StartServer;
        if (params.Host[Memory::ArraySize(params.Host) - 1] != '\0')
        {
            err = Error::InvalidValue;
            break;
        }

        AString host(params.Host, Memory::ArraySize(params.Host) - 1, err);
        if (err != Error::Success)
        {
            break;
        }

        err = StartServer(host, params.Port);
        break;
    }
    case IOCTL_KSTOR_STOP_SERVER:
        err = StopServer();
        break;
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