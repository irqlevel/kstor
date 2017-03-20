#include "control_device.h"
#include "volume.h"

#include <core/trace.h>
#include <core/copy_user.h>
#include <core/time.h>
#include <core/unique_ptr.h>
#include <core/trace.h>
#include <core/auto_lock.h>
#include <core/shared_auto_lock.h>
#include <core/task.h>
#include <core/btree.h>
#include <core/random.h>

#include <include/ctl.h>

namespace KStor 
{

ControlDevice* ControlDevice::Device = nullptr;

ControlDevice::ControlDevice(Core::Error& err)
    : MiscDevice(KSTOR_CONTROL_DEVICE, err)
    , Rng(err)
{
}

Core::Error ControlDevice::Mount(const Core::AString& deviceName, bool format, Guid& volumeId)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() != nullptr)
    {
        return Core::Error::AlreadyExists;
    }

    Core::Error err;
    VolumeRef = Core::MakeShared<Volume, Core::Memory::PoolType::Kernel>(deviceName, err);
    if (VolumeRef.Get() == nullptr)
    {
        trace(0, "CtrlDev 0x%p can't allocate device", this);
        err.SetNoMemory();
        return err;
    }

    if (!err.Ok())
    {
        trace(0, "CtrlDev 0x%p device init err %d", this, err.GetCode());
        return err;
    }

    if (format)
    {
        err = VolumeRef->Format();
        if (!err.Ok())
        {
            trace(0, "CtrlDev 0x%p device format err %d", this, err.GetCode());
            return err;
        }
    }

    err = VolumeRef->Load();
    if (!err.Ok())
    {
        trace(0, "CtrlDev 0x%p device load err %d", this, err.GetCode());
        return err;
    }

    volumeId = VolumeRef->GetVolumeId();
    return Core::Error::Success;
}

Core::Error ControlDevice::Unmount(const Guid& volumeId)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    if (VolumeRef->GetVolumeId() == volumeId)
    {
        auto err = VolumeRef->Unload();
        VolumeRef.Reset();
        return err;
    }

    return Core::Error::NotFound;
}

Core::Error ControlDevice::Unmount(const Core::AString& deviceName)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    if (VolumeRef->GetDeviceName().Compare(deviceName) == 0)
    {
        auto err = VolumeRef->Unload();
        VolumeRef.Reset();
        return err;
    }

    return Core::Error::NotFound;
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

Core::Error ControlDevice::TestBtree()
{
    Core::Error err;

    trace(1, "Test btree");

    size_t keyCount = 10000;
    Core::Vector<uint64_t> key;
    if (!key.ReserveAndUse(keyCount))
        return Core::Error::NoMemory;

    Core::Vector<uint64_t> value;
    if (!value.ReserveAndUse(keyCount))
        return Core::Error::NoMemory;

    for (size_t i = 0; i < key.GetSize(); i++)
        key[i] = Core::Random::GetUint64();
    for (size_t i = 0; i < value.GetSize(); i++)
        value[i] = Core::Random::GetUint64();

    Core::Btree<uint64_t, uint64_t, 3> tree;

    /* insert all keys */
    for (size_t i = 0; i < key.GetSize(); i++)
    {
        if (!tree.Insert(key[i], value[i]))
            return Core::Error::Unsuccessful;
    }

    if (!tree.Check())
        return Core::Error::Unsuccessful;

    /* lookup all keys */
    for (size_t i = 0; i < key.GetSize(); i++)
    {
        bool exist;
        auto foundValue = tree.Lookup(key[i], exist);
        if (!exist)
            return Core::Error::Unsuccessful;
        if (foundValue != value[i])
            return Core::Error::Unsuccessful;
    }

    /* delete all keys */
    key.Shuffle();
    for (size_t i = 0; i < key.GetSize() / 2; i++)
    {
        if (!tree.Delete(key[i]))
            return Core::Error::Unsuccessful;
    }

    if (!tree.Check())
        return Core::Error::Unsuccessful;

    for (size_t i = key.GetSize() / 2; i < key.GetSize(); i++)
    {
        if (!tree.Delete(key[i]))
            return Core::Error::Unsuccessful;
    }

    if (!tree.Check())
        return Core::Error::Unsuccessful;

    /* lookup all keys */
    key.Shuffle();
    for (size_t i = 0; i < key.GetSize(); i++)
    {
        bool exist;
        tree.Lookup(key[i], exist);
        if (exist)
            return Core::Error::Unsuccessful;
    }

    trace(1, "Test btree complete");

    return Core::Error::Success;
}

Core::Error ControlDevice::RunTest(unsigned int testId)
{
    Core::Error err;

    trace(1, "Run test %d", testId);

    switch (testId)
    {
    case Api::TestJournal:
    {
        Core::AutoLock lock(VolumeLock);

        if (VolumeRef.Get() == nullptr)
        {
            err =  Core::Error::NotFound;
            break;
        }

        err = VolumeRef->TestJournal();
        break;
    }
    case Api::TestBtree:
    {
        TestBtree();
        break;
    }
    default:
        err = Core::Error::InvalidValue;
        break;
    }

    trace(1, "Run test %d err %d", testId, err.GetCode());

    return err;
}

Core::Error ControlDevice::GetTaskStack(int pid, char *stack, unsigned long len)
{
    Core::Error err;

    trace(1, "Get task %d stack", pid);


    Core::Task task(pid, err);
    if (!err.Ok())
        goto out;

    {
        Core::AString stackStr;

        err = task.DumpStack(stackStr);
        if (!err.Ok())
            goto out;

        Core::Memory::SnPrintf(stack, len, "%s", stackStr.GetConstBuf());
    }

out:
    trace(1, "Get task %d stack result %d", pid, err.GetCode());

    return err;
}

Core::Error ControlDevice::Ioctl(unsigned int code, unsigned long arg)
{
    trace(3, "Ioctl 0x%x arg 0x%lx", code, arg);

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
    case IOCTL_KSTOR_TEST:
    {
        auto& params = cmd->Union.Test;
        err = RunTest(params.TestId);
        break;
    }
    case IOCTL_KSTOR_GET_TASK_STACK:
    {
        auto& params = cmd->Union.GetTaskStack;
        err = GetTaskStack(params.Pid, params.Stack, sizeof(params.Stack));
        break;
    }
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

    trace(1, "Ioctl 0x%x result %d", code, err.GetCode());
    return err;
}

ControlDevice::~ControlDevice()
{
}

Core::Error ControlDevice::ChunkCreate(const Guid& chunkId)
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkCreate(chunkId);
}

Core::Error ControlDevice::ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkWrite(chunkId, data);
}

Core::Error ControlDevice::ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkRead(chunkId, data);
}

Core::Error ControlDevice::ChunkDelete(const Guid& chunkId)
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkDelete(chunkId);
}

ControlDevice* ControlDevice::Get()
{
    return Device;
}

Core::Error ControlDevice::Create()
{
    Core::Error err;

    if (Device != nullptr)
        return Core::Error::InvalidState;

    Device = new (Core::Memory::PoolType::Kernel) ControlDevice(err);
    if (Device == nullptr)
    {
        err.SetNoMemory();
        return err;
    }

    if (!err.Ok())
    {
        delete Device;
        Device = nullptr;
        return err;
    }

    return err;
}

void ControlDevice::Delete()
{
    if (Device != nullptr)
    {
        delete Device;
        Device = nullptr;
    }
}

}