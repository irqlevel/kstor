#include "init.h"

#include "control_device.h"

#include <core/trace.h>
#include <include/ctl.h>

int KStorInit(void)
{
    Core::Trace::SetLevel(1);

    trace(1, "initing");

    Core::Error err = KStor::ControlDevice::Create();
  
    trace(1, "inited, err %d", err.GetCode());
    return err.GetCode();
}

void KStorDeinit(void)
{
    trace(1, "deiniting");
    KStor::ControlDevice::Delete();
    trace(1,"deinited");
}