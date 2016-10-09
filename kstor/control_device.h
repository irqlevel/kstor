#pragma once

#include <core/misc_device.h>
#include <core/random.h>

class ControlDevice : public MiscDevice
{
public:
    ControlDevice(Error& err);

    Error Ioctl(unsigned int code, unsigned long arg) override;

    virtual ~ControlDevice();

private:

    Random Rng;
};
