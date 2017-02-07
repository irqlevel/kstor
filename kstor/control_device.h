#pragma once

#include <core/misc_device.h>
#include <core/random.h>
#include <core/rwsem.h>
#include <core/list.h>

#include "super_block.h"
#include "server.h"

namespace KStor 
{

class ControlDevice : public Core::MiscDevice
{
public:
    ControlDevice(Core::Error& err);

    Core::Error Ioctl(unsigned int code, unsigned long arg) override;

    Core::Error Mount(const Core::AString& deviceName, bool format, unsigned long& deviceId);

    SuperBlockPtr LookupMount(unsigned long deviceId);
    SuperBlockPtr LookupMount(const Core::AString& deviceName);

    Core::Error Unmount(unsigned long deviceId);
    Core::Error Unmount(const Core::AString& deviceName);

    virtual ~ControlDevice();

    Core::Error StartServer(const Core::AString& host, unsigned short port);
    Core::Error StopServer();

private:
    Server Srv;
    Core::Random Rng;
    Core::RWSem SuperBlockListLock;
    Core::LinkedList<SuperBlockPtr, Core::Memory::PoolType::Kernel> SuperBlockList;
};

}
