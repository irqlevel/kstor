#include "super_block.h"

#include <core/trace.h>
#include <core/bio.h>

SuperBlock::SuperBlock(const char* deviceName, bool format, Error& err)
    : DeviceName(deviceName, err)
    , BDev(DeviceName, err)
{
    if (err != Error::Success)
    {
        goto out;
    }

    trace(1, "Device 0x%p size 0x%llx", this, BDev.GetSize());

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

SuperBlock::~SuperBlock()
{
    trace(1, "Device 0x%p dtor", this);
}

Error SuperBlock::Format()
{
    Error err;

    Page page(Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate page");
        return err;
    }

    page.Zero();

    Bio bio(BDev, page, 0, err, true);
    if (err != Error::Success)
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

Error SuperBlock::Load()
{
    Error err;

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

const AString& SuperBlock::GetName() const
{
    return DeviceName;
}

BlockDevice& SuperBlock::GetBDev()
{
    return BDev;
}
