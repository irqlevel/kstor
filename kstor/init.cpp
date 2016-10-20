#include "init.h"
#include "test.h"

#include "control_device.h"

#include <core/trace.h>
#include <include/ctl.h>

static ControlDevice* ControlDevicePtr = nullptr;

int KStorInit(void)
{
    trace(1, "initing");

//  run_tests();

    Error err;
    ControlDevicePtr = new (Memory::PoolType::Kernel) ControlDevice(err);
    if (ControlDevicePtr == nullptr)
    {
        err = Error::NoMemory;
        return err.GetCode();
    }

    if (err != Error::Success)
    {
        delete ControlDevicePtr;
        ControlDevicePtr = nullptr;
        return err.GetCode();
    }
  
    trace(1, "inited");
    return 0;
}

void KStorDeinit(void)
{
    trace(1, "deiniting");
    if (ControlDevicePtr != nullptr)
    {
        delete ControlDevicePtr;
        ControlDevicePtr = nullptr;
    }
    trace(1,"deinited");
}
