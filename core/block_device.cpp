#include "block_device.h"

BlockDevice::BlockDevice(const AString& deviceName, Error& err)
    : BDevPtr(nullptr)
    , Mode(KAPI_BDEV_MODE_READ|KAPI_BDEV_MODE_WRITE|KAPI_BDEV_MODE_EXCLUSIVE)
{
    if (err != Error::Success)
    {
        return;
    }

    int rc = get_kapi()->bdev_get_by_path(deviceName.GetBuf(),
        Mode, this, &BDevPtr);
    if (rc != 0)
    {
        err = Error(rc);
        return;
    }
}

BlockDevice::~BlockDevice()
{
    if (BDevPtr != nullptr)
    {
        get_kapi()->bdev_put(BDevPtr, Mode);
        BDevPtr = nullptr;
    }
}
