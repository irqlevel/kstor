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

class Chunk
{
public:
    Chunk(const Guid chunkId, unsigned char data[Api::ChunkSize])
        : ChunkId(chunkId)
    {
        Core::Memory::MemCpy(Data, data, sizeof(Data));
    }

    virtual ~Chunk(){}

    Guid ChunkId;
    unsigned char Data[Api::ChunkSize];
private:
    Chunk(const Chunk& other) = delete;
    Chunk(Chunk&& other) = delete;
    Chunk& operator=(const Chunk& other) = delete;
    Chunk& operator=(Chunk&& other) = delete;
};

typedef Core::SharedPtr<Chunk, Core::Memory::PoolType::Kernel> ChunkPtr;

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

    Core::Error ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize]);
    Core::Error ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize]);
    Core::Error ChunkDelete(const Guid& chunkId);

    static ControlDevice* Get();
    static Core::Error Create();
    static void Delete();

private:
    Server Srv;
    Core::Random Rng;
    Core::RWSem VolumeListLock;
    Core::LinkedList<VolumePtr, Core::Memory::PoolType::Kernel> VolumeList;
    Core::HashTable<Guid, ChunkPtr, Core::RWSem, Core::Memory::PoolType::Kernel, 512> ChunkTable;
    static ControlDevice* Device;
};

}
