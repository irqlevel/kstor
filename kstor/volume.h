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

const unsigned int VolumeStateNew = 1;
const unsigned int VolumeStateRunning = 2;
const unsigned int VolumeStateStopping = 3;
const unsigned int VolumeStateStopped = 4;

class Volume
{
public:
    Volume(const Core::AString& deviceName, Core::Error& err);
    virtual ~Volume();

    Core::Error Format();
    Core::Error Load();
    Core::Error Unload();
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

    Core::Error TestJournal();

private:
    Core::AString DeviceName;
    Core::BlockDevice Device;
    Guid VolumeId;
    Core::HashTable<Guid, ChunkPtr, Core::RWSem, Core::Memory::PoolType::Kernel, 512> ChunkTable;
    uint64_t Size;
    uint64_t BlockSize;
    Journal JournalObj;
    Core::RWSem Lock;
    unsigned int State;
};

typedef Core::SharedPtr<Volume, Core::Memory::PoolType::Kernel> VolumePtr;

}