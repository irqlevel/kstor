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
#include "journal.h"

namespace KStor 
{

class Volume
{
public:
    Volume(const Core::AString& deviceName, Core::Error& err);
    virtual ~Volume();

    Core::Error Format(unsigned long blockSize);
    Core::Error Load();

    const Guid& GetVolumeId() const;

    uint64_t GetSize() const;

    uint64_t GetBlockSize() const;

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
    Guid VolumeId;
    Core::HashTable<Guid, ChunkPtr, Core::RWSem, Core::Memory::PoolType::Kernel, 512> ChunkTable;
    uint64_t Size;
    uint64_t BlockSize;
    Journal JournalObj;
};

typedef Core::SharedPtr<Volume, Core::Memory::PoolType::Kernel> VolumePtr;

}