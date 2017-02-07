#include "control_device.h"
#include "volume.h"

#include <core/trace.h>
#include <core/copy_user.h>
#include <core/time.h>
#include <core/unique_ptr.h>
#include <core/trace.h>
#include <core/auto_lock.h>
#include <core/shared_auto_lock.h>

#include <include/ctl.h>

namespace KStor 
{

ControlDevice::ControlDevice(Core::Error& err)
    : MiscDevice(KSTOR_CONTROL_DEVICE, err)
    , Rng(err)
{
}

Core::Error ControlDevice::Mount(const Core::AString& deviceName, bool format, Guid& volumeId)
{
    Core::Error err;

    VolumePtr volume = Core::MakeShared<Volume, Core::Memory::PoolType::Kernel>(deviceName, format, err);
    if (volume.Get() == nullptr)
    {
        trace(1, "CtrlDev 0x%p can't allocate device", this);
        err.SetNoMemory();
        return err;
    }

    if (!err.Ok())
    {
        trace(1, "CtrlDev 0x%p device init err %d", this, err.GetCode());
        return err;
    }

    Core::AutoLock lock(VolumeListLock);
    if (!VolumeList.AddHead(volume))
    {
        trace(1, "CtrlDev 0x%p can't add device into list");
        err.SetNoMemory();
        return err;
    }

    volumeId = volume->GetVolumeId();
    return Core::Error::Success;
}

VolumePtr ControlDevice::LookupMount(const Guid& volumeId)
{
    Core::SharedAutoLock lock(VolumeListLock);

    auto it = VolumeList.GetIterator();
    while (it.IsValid())
    {
        auto volume = it.Get();
        if (volume->GetVolumeId() == volumeId)
        {
            return volume;
        }
        it.Next();
    }

    return VolumePtr();
}

VolumePtr ControlDevice::LookupMount(const Core::AString& deviceName)
{
    Core::SharedAutoLock lock(VolumeListLock);

    auto it = VolumeList.GetIterator();
    while (it.IsValid())
    {
        auto volume = it.Get();
        if (volume->GetDeviceName().Compare(deviceName) == 0)
        {
            return volume;
        }
        it.Next();
    }

    return VolumePtr();
}

Core::Error ControlDevice::Unmount(const Guid& volumeId)
{
    VolumePtr volume = LookupMount(volumeId);
    if (volume.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    Core::AutoLock lock(VolumeListLock);
    auto it = VolumeList.GetIterator();
    while (it.IsValid())
    {
        auto volume = it.Get();
        if (volume->GetVolumeId() == volumeId)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Core::Error::Success;
}

Core::Error ControlDevice::Unmount(const Core::AString& deviceName)
{
    VolumePtr volume = LookupMount(deviceName);
    if (volume.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    Core::AutoLock lock(VolumeListLock);
    auto it = VolumeList.GetIterator();
    while (it.IsValid())
    {
        auto volume = it.Get();
        if (volume->GetDeviceName().Compare(deviceName) == 0)
        {
            it.Erase();
        } else
        {
            it.Next();
        }
    }

    return Core::Error::Success;
}

Core::Error ControlDevice::StartServer(const Core::AString& host, unsigned short port)
{
    return Srv.Start(host, port);
}

Core::Error ControlDevice::StopServer()
{
    Srv.Stop();
    return Core::Error::Success;
}

Core::Error ControlDevice::Ioctl(unsigned int code, unsigned long arg)
{
    trace(1, "Ioctl 0x%x arg 0x%lx", code, arg);

    Core::Error err;
    Core::UniquePtr<Control::Cmd> cmd(new (Core::Memory::PoolType::Kernel) Control::Cmd);
    if (cmd.Get() == nullptr)
    {
        trace(0, "Can't allocate memory");
        goto cleanup;
    }

    err = Core::CopyFromUser(cmd.Get(), reinterpret_cast<Control::Cmd*>(arg));
    if (!err.Ok())
    {
        trace(0, "Can't copy cmd from user");
        goto cleanup;
    }

    switch (code)
    {
    case IOCTL_KSTOR_GET_TIME:
        cmd->Union.GetTime.Time = Core::Time::GetTime();
        break;
    case IOCTL_KSTOR_GET_RANDOM_ULONG:
        cmd->Union.GetRandomUlong.Value = Rng.GetUlong();
        break;
    case IOCTL_KSTOR_MOUNT:
    {
        auto& params = cmd->Union.Mount;
        if (params.DeviceName[Core::Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString deviceName(params.DeviceName, Core::Memory::ArraySize(params.DeviceName) - 1, err);
        if (!err.Ok())
        {
            break;
        }

        Guid volumeId;
        err = Mount(deviceName, params.Format, volumeId);
        if (err.Ok()) {
            params.VolumeId = volumeId.GetContent();
        }
        break;
    }
    case IOCTL_KSTOR_UNMOUNT:
        err = Unmount(cmd->Union.Unmount.VolumeId);
        break;
    case IOCTL_KSTOR_UNMOUNT_BY_NAME:
    {
        auto& params = cmd->Union.UnmountByName;
        if (params.DeviceName[Core::Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString deviceName(params.DeviceName, Core::Memory::ArraySize(params.DeviceName) - 1, err);
        if (!err.Ok())
        {
            break;
        }

        err = Unmount(deviceName);
        break;
    }
    case IOCTL_KSTOR_START_SERVER:
    {
        auto& params = cmd->Union.StartServer;
        if (params.Host[Core::Memory::ArraySize(params.Host) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString host(params.Host, Core::Memory::ArraySize(params.Host) - 1, err);
        if (!err.Ok())
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
        err = Core::Error::UnknownCode;
        break;
    }

    if (!err.Ok())
    {
        goto cleanup;
    }

    err = Core::CopyToUser(reinterpret_cast<Control::Cmd*>(arg), cmd.Get());
    if (!err.Ok())
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

}