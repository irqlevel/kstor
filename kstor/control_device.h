#pragma once

#include <core/misc_device.h>
#include <core/random.h>
#include <core/rwsem.h>
#include <core/list.h>
#include <core/hash_table.h>
#include <core/vector.h>

#include "volume.h"
#include "server.h"
#include "guid.h"
#include "api.h"

namespace KStor 
{

class ControlDevice : public Core::MiscDevice
{
public:
    ControlDevice(Core::Error& err);

    Core::Error Ioctl(unsigned int code, unsigned long arg) override;

    Core::Error Mount(const Core::AString& deviceName, bool format, uint64_t blockSize, Guid& volumeId);
    Core::Error Unmount(const Guid& volumeId);
    Core::Error Unmount(const Core::AString& deviceName);

    virtual ~ControlDevice();

    Core::Error StartServer(const Core::AString& host, unsigned short port);
    Core::Error StopServer();

    Core::Error ChunkCreate(const Guid& chunkId);
    Core::Error ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize]);
    Core::Error ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize]);
    Core::Error ChunkDelete(const Guid& chunkId);

    static ControlDevice* Get();
    static Core::Error Create();
    static void Delete();

    Core::Error RunTest(unsigned int testId);

private:
    Server Srv;
    Core::Random Rng;
    Core::RWSem VolumeLock;
    VolumePtr VolumeRef;
    static ControlDevice* Device;
};

}
