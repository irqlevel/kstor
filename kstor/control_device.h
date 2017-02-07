#pragma once

#include <core/misc_device.h>
#include <core/random.h>
#include <core/rwsem.h>
#include <core/list.h>

#include "volume.h"
#include "server.h"
#include "guid.h"

namespace KStor 
{

class ControlDevice : public Core::MiscDevice
{
public:
    ControlDevice(Core::Error& err);

    Core::Error Ioctl(unsigned int code, unsigned long arg) override;

    Core::Error Mount(const Core::AString& deviceName, bool format, Guid& volumeId);

    VolumePtr LookupMount(const Guid& volumeId);
    VolumePtr LookupMount(const Core::AString& deviceName);

    Core::Error Unmount(const Guid& volumeId);
    Core::Error Unmount(const Core::AString& deviceName);

    virtual ~ControlDevice();

    Core::Error StartServer(const Core::AString& host, unsigned short port);
    Core::Error StopServer();

private:
    Server Srv;
    Core::Random Rng;
    Core::RWSem VolumeListLock;
    Core::LinkedList<VolumePtr, Core::Memory::PoolType::Kernel> VolumeList;
};

}
