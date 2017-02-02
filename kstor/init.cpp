#include "init.h"

#include "control_device.h"

#include <core/trace.h>
#include <include/ctl.h>

namespace KStor 
{

static ControlDevice* ControlDevicePtr = nullptr;

}

int KStorInit(void)
{
    trace(1, "initing");

    Core::Error err;
    KStor::ControlDevicePtr = new (Core::Memory::PoolType::Kernel) KStor::ControlDevice(err);
    if (KStor::ControlDevicePtr == nullptr)
    {
        err = Core::Error::NoMemory;
        return err.GetCode();
    }

    if (err != Core::Error::Success)
    {
        delete KStor::ControlDevicePtr;
        KStor::ControlDevicePtr = nullptr;
        return err.GetCode();
    }
  
    trace(1, "inited");
    return 0;
}

void KStorDeinit(void)
{
    trace(1, "deiniting");
    if (KStor::ControlDevicePtr != nullptr)
    {
        delete KStor::ControlDevicePtr;
        KStor::ControlDevicePtr = nullptr;
    }
    trace(1,"deinited");
}