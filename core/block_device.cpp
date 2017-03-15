#include "block_device.h"
#include "trace.h"

namespace Core
{

BlockDevice::BlockDevice(const AString& deviceName, Error& err)
    : BDevPtr(nullptr)
    , Mode(KAPI_BDEV_MODE_READ|KAPI_BDEV_MODE_WRITE|KAPI_BDEV_MODE_EXCLUSIVE)
{
    if (!err.Ok())
    {
        return;
    }

    int rc = get_kapi()->bdev_get_by_path(deviceName.GetConstBuf(),
        Mode, this, &BDevPtr);
    if (rc != 0)
    {
        trace(0, "Can't get bdev %s, err %d", deviceName.GetConstBuf(), rc);
        err = Error(rc);
        return;
    }

    trace(4, "Bdev 0x%p bdev 0x%p ctor", this, BDevPtr);
}

void* BlockDevice::GetBdev()
{
    return BDevPtr;
}

unsigned long long BlockDevice::GetSize() const
{
    return get_kapi()->bdev_get_size(BDevPtr);
}

BlockDevice::~BlockDevice()
{
    trace(4, "Bdev 0x%p bdev 0x%p dtor", this, BDevPtr);

    if (BDevPtr != nullptr)
    {
        get_kapi()->bdev_put(BDevPtr, Mode);
        BDevPtr = nullptr;
    }
}

}