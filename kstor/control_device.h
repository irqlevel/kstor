#pragma once

#include <core/misc_device.h>
#include <core/random.h>
#include <core/rwsem.h>
#include <core/list.h>

#include "super_block.h"
#include "server.h"

class ControlDevice : public MiscDevice
{
public:
    ControlDevice(Error& err);

    Error Ioctl(unsigned int code, unsigned long arg) override;

    Error Mount(const AString& deviceName, bool format, unsigned long& deviceId);

    SuperBlockRef LookupMount(unsigned long deviceId);
    SuperBlockRef LookupMount(const AString& deviceName);

    Error Unmount(unsigned long deviceId);
    Error Unmount(const AString& deviceName);

    virtual ~ControlDevice();

    Error StartServer(const AString& host, unsigned short port);
    Error StopServer();

private:
    Server Srv;
    Random Rng;
    RWSem SuperBlockListLock;
    LinkedList<SuperBlockRef, Memory::PoolType::Kernel> SuperBlockList;
};
