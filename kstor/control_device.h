#pragma once

#include <core/misc_device.h>
#include <core/random.h>
#include <core/rwsem.h>
#include <core/list.h>

#include "device.h"

class ControlDevice : public MiscDevice
{
public:
    ControlDevice(Error& err);

    Error Ioctl(unsigned int code, unsigned long arg) override;

    Error DeviceAdd(const char* deviceName, bool format, unsigned long& deviceId);
    DeviceRef DeviceLookup(unsigned long deviceId);
    Error DeviceRemove(unsigned long deviceId);

    virtual ~ControlDevice();

private:

    Random Rng;

    RWSem DeviceListLock;
    LinkedList<DeviceRef, Memory::PoolType::Kernel> DeviceList;
};
