#include "control_device.h"

#include <core/trace.h>
#include <core/copy_user.h>
#include <core/time.h>
#include <core/unique_ptr.h>

#include <include/ctl.h>

ControlDevice::ControlDevice(Error& err)
    : MiscDevice(KSTOR_CONTROL_DEVICE, err)
    , Rng(err)
{
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