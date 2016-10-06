#pragma once

#include <core/misc_device.h>

class ControlDevice : public MiscDevice
{
public:
    ControlDevice(Error& err);

    Error Ioctl(unsigned int code, unsigned long arg) override;

    virtual ~ControlDevice();

private:

};
