#include "volume.h"

#include <core/trace.h>
#include <core/bitops.h>
#include <core/hex.h>
#include <core/xxhash.h>

namespace KStor
{

Volume::Volume(const Core::AString& deviceName, Core::Error& err)
    : DeviceName(deviceName, err)
    , Device(DeviceName, err)
    , Size(0)
    , BlockSize(0)
    , JournalObj(*this)
{
    if (!err.Ok())
    {
        return;
    }

    trace(1, "Volume 0x%p name %s size %llu ctor",
        this, deviceName.GetBuf(), Device.GetSize());
}

Volume::~Volume()
{
    trace(1, "Volume 0x%p dtor", this);
}

Core::Error Volume::Format(unsigned long blockSize)
{
    if (blockSize == 0 || blockSize % Api::PageSize)
        return Core::Error::InvalidValue;

    uint64_t size = Device.GetSize();
    if (size == 0 || size % blockSize)
        return Core::Error::InvalidValue;

    BlockSize = blockSize;
    Size = size;

    Core::Error err = JournalObj.Format(1, (size / 10) / blockSize);
    if (!err.Ok())
        return err;

    auto page = Core::Page<Core::Memory::PoolType::Kernel>::Create(err);
    if (!err.Ok())
        return err;
    page->Zero();

    VolumeId.Generate();
    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    header->Magic = Core::BitOps::CpuToLe32(Api::VolumeMagic);
    header->VolumeId = VolumeId.GetContent();
    header->Size = Core::BitOps::CpuToLe64(size);
    header->BlockSize = Core::BitOps::CpuToLe64(blockSize);
    header->JournalSize = Core::BitOps::CpuToLe64(JournalObj.GetSize());

    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), header->Hash);

    err = Device.Write<Core::Memory::PoolType::Kernel>(page, 0);

    trace(1, "Volume 0x%p write header, err %d", this, err.GetCode());

    return err;
}

Core::Error Volume::Load()
{
    Core::Error err;
    auto page = Core::Page<Core::Memory::PoolType::Kernel>::Create(err);
    if (!err.Ok())
        return err;

    err = Device.Read<Core::Memory::PoolType::Kernel>(page, 0);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p read header, err %d", this, err.GetCode());
        return err;
    }

    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    if (Core::BitOps::Le32ToCpu(header->Magic) != Api::VolumeMagic)
    {
        trace(0, "Volume 0x%p bad header magic", this);
        return Core::Error::BadMagic;
    }

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), hash);

    if (!Core::Memory::ArrayEqual(header->Hash, hash))
    {
        trace(0, "Volume 0x%p bad header hash", this);
        return Core::Error::DataCorrupt;
    }

    uint64_t size = Core::BitOps::Le64ToCpu(header->Size);
    uint64_t blockSize = Core::BitOps::Le64ToCpu(header->BlockSize);

    if (blockSize == 0 || blockSize % Api::PageSize || size % blockSize)
    {
        trace(0, "Volume 0x%p bad block size %llu", this, blockSize);
        return Core::Error::BadSize;
    }

    if (size == 0 || size != Device.GetSize())
    {
        trace(0, "Volume 0x%p bad size %llu", this, size);
        return Core::Error::BadSize;
    }

    uint64_t journalSize = Core::BitOps::Le64ToCpu(header->JournalSize);
    err = JournalObj.Load(1);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p can't load journal, err %d", this, err.GetCode());
        return err;
    }

    if (JournalObj.GetSize() != journalSize)
    {
        trace(0, "Volume 0x%p bad journal size %llu vs. %llu",
            this, JournalObj.GetSize(), journalSize);
        return Core::Error::BadSize;
    }

    Size = size;
    BlockSize = blockSize;

    VolumeId.SetContent(header->VolumeId);

    trace(1, "Volume 0x%p load volumeId %s size %llu blockSize %llu",
        this, VolumeId.ToString().GetBuf(), Size, BlockSize);

    return err;
}

const Guid& Volume::GetVolumeId() const
{
    return VolumeId;
}

uint64_t Volume::GetSize() const
{
    return Size;
}

uint64_t Volume::GetBlockSize() const
{
    return BlockSize;
}

const Core::AString& Volume::GetDeviceName() const
{
    return DeviceName;
}

Core::BlockDevice& Volume::GetDevice()
{
    return Device;
}

Core::Error Volume::ChunkCreate(const Guid& chunkId)
{
    trace(1, "Chunk %s create", chunkId.ToString().GetBuf());

    auto chunk = ChunkTable.Get(chunkId);
    if (chunk.Get() != nullptr)
    {
        return Core::Error::AlreadyExists;
    }

    chunk = Core::MakeShared<Chunk, Core::Memory::PoolType::Kernel>(chunkId);
    if (chunk.Get() == nullptr)
        return Core::Error::NoMemory;

    if (!ChunkTable.Insert(chunk->ChunkId, chunk))
        return Core::Error::NoMemory;

    return Core::Error::Success;
}

Core::Error Volume::ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    trace(1, "Chunk %s write", chunkId.ToString().GetBuf());

    auto chunk = ChunkTable.Get(chunkId);
    if (chunk.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    Core::Memory::MemCpy(chunk->Data, data, sizeof(chunk->Data));

    trace(3, "Chunk %s write %s size %lu", chunkId.ToString().GetBuf(),
        Core::Hex::Encode(chunk->Data, 10).GetBuf(), sizeof(chunk->Data));

    return Core::Error::Success;
}

Core::Error Volume::ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    trace(1, "Chunk %s read", chunkId.ToString().GetBuf());

    auto chunk = ChunkTable.Get(chunkId);
    if (chunk.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    Core::Memory::MemCpy(data, chunk->Data, sizeof(chunk->Data));

    trace(3, "Chunk %s read %s size %lu", chunkId.ToString().GetBuf(),
        Core::Hex::Encode(data, 10).GetBuf(), sizeof(chunk->Data));

    return Core::Error::Success;
}

Core::Error Volume::ChunkDelete(const Guid& chunkId)
{
    trace(1, "Chunk %s delete", chunkId.ToString().GetBuf());

    if (ChunkTable.Remove(chunkId))
        return Core::Error::Success;

    return Core::Error::NotFound;
}

Core::Error Volume::ChunkLookup(const Guid& chunkId)
{
    trace(1, "Chunk %s lookup", chunkId.ToString().GetBuf());

    if (!ChunkTable.CheckExist(chunkId))
        return Core::Error::NotFound;
    return Core::Error::Success;
}

}