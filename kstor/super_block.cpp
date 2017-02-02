#include "super_block.h"

#include <core/trace.h>
#include <core/bio.h>

namespace KStor
{

SuperBlock::SuperBlock(const Core::AString& deviceName, bool format, Core::Error& err)
    : DeviceName(deviceName, err)
    , BDev(DeviceName, err)
{
    if (!err.Ok())
    {
        goto out;
    }

    trace(1, "Device 0x%p size 0x%llx", this, BDev.GetSize());

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

SuperBlock::~SuperBlock()
{
    trace(1, "Device 0x%p dtor", this);
}

Core::Error SuperBlock::Format()
{
    Core::Error err;

    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
    {
        trace(0, "Can't allocate page");
        return err;
    }

    page.Zero();

    Core::Bio bio(BDev, page, 0, err, true);
    if (!err.Ok())
    {
        trace(0, "Can't init bio, err %d", err.GetCode());
        return err;
    }

    bio.Submit();
    bio.Wait();

    err = bio.GetError();

    trace(1, "Bio write result %d", err.GetCode());

    return err;
}

Core::Error SuperBlock::Load()
{
    Core::Error err;

    trace(1, "Device 0x%p load err %d", this, err.GetCode());

    return err;
}

unsigned long SuperBlock::GetId() const
{
    return reinterpret_cast<unsigned long>(this);
}

unsigned long long SuperBlock::GetSize() const
{
    return BDev.GetSize();
}

const Core::AString& SuperBlock::GetName() const
{
    return DeviceName;
}

Core::BlockDevice& SuperBlock::GetBDev()
{
    return BDev;
}

}