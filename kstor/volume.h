#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>
#include <core/page.h>
#include <core/hash_table.h>
#include <core/rwsem.h>

#include "guid.h"
#include "chunk.h"

namespace KStor 
{

class Volume
{
public:
    Volume(const Core::AString& deviceName, bool format, Core::Error& err);
    virtual ~Volume();

    Core::Error Format();
    Core::Error Load();

    const Guid& GetVolumeId() const;

    unsigned long long GetSize() const;

    const Core::AString& GetDeviceName() const;

    Core::BlockDevice& GetDevice();

    Core::Error ChunkCreate(const Guid& chunkId);

    Core::Error ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize]);

    Core::Error ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize]);

    Core::Error ChunkDelete(const Guid& chunkId);

    Core::Error ChunkLookup(const Guid& chunkId);

private:
    Core::AString DeviceName;
    Core::BlockDevice Device;
    Core::Page HeaderPage;
    Guid VolumeId;
    Core::HashTable<Guid, ChunkPtr, Core::RWSem, Core::Memory::PoolType::Kernel, 512> ChunkTable;
};

typedef Core::SharedPtr<Volume, Core::Memory::PoolType::Kernel> VolumePtr;

}