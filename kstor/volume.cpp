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
    , BlockSize(Api::PageSize)
    , JournalObj(*this)
    , State(VolumeStateNew)
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
    Unload();
    trace(1, "Volume 0x%p dtor", this);
}

Core::Error Volume::Format()
{
    Core::AutoLock lock(Lock);

    if (State != VolumeStateNew)
        return Core::Error::InvalidState;

    uint64_t size = Device.GetSize();
    if (size == 0 || size % BlockSize)
        return Core::Error::InvalidValue;

    Size = size;

    Core::Error err = JournalObj.Format(1, (size / 10) / BlockSize);
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
    header->JournalSize = Core::BitOps::CpuToLe64(JournalObj.GetSize());

    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), header->Hash);

    err = Core::BioList<Core::Memory::PoolType::Kernel>(Device).SubmitWaitResult(page, 0, true, true);

    trace(1, "Volume 0x%p write header, err %d", this, err.GetCode());

    return err;
}

Core::Error Volume::Load()
{
    Core::AutoLock lock(Lock);
    if (State != VolumeStateNew)
        return Core::Error::InvalidState;

    Core::Error err;
    auto page = Core::Page<Core::Memory::PoolType::Kernel>::Create(err);
    if (!err.Ok())
        return err;

    err = Core::BioList<Core::Memory::PoolType::Kernel>(Device).SubmitWaitResult(page, 0, false);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p read header, err %d", this, err.GetCode());
        return err;
    }

    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    if (Core::BitOps::Le32ToCpu(header->Magic) != Api::VolumeMagic)
    {
        trace(0, "Volume 0x%p bad header magic 0x%x", this, Core::BitOps::Le32ToCpu(header->Magic));
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
    if (size % BlockSize)
    {
        trace(0, "Volume 0x%p bad size %llu", this, size);
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

    VolumeId.SetContent(header->VolumeId);

    State = VolumeStateRunning;
    trace(1, "Volume 0x%p load volumeId %s size %llu blockSize %llu",
        this, VolumeId.ToString().GetBuf(), Size, BlockSize);

    return err;
}

Core::Error Volume::Unload()
{
    trace(1, "Volume 0x%p unload", this);

    Core::AutoLock lock(Lock);
    if (State == VolumeStateStopped)
        return Core::Error::Success;
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

    State = VolumeStateStopping;
    auto err = JournalObj.Unload();
    if (!err.Ok())
        return err;

    auto page = Core::Page<Core::Memory::PoolType::Kernel>::Create(err);
    if (!err.Ok())
        return err;

    page->Zero();

    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    header->Magic = Core::BitOps::CpuToLe32(Api::VolumeMagic);
    header->VolumeId = VolumeId.GetContent();
    header->Size = Core::BitOps::CpuToLe64(Size);
    header->JournalSize = Core::BitOps::CpuToLe64(JournalObj.GetSize());

    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), header->Hash);

    err = Core::BioList<Core::Memory::PoolType::Kernel>(Device).SubmitWaitResult(page, 0, true, true);

    State = VolumeStateStopped;

    trace(1, "Volume 0x%p unload, err %d", this, err.GetCode());

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
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

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
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

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
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

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
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

    trace(1, "Chunk %s delete", chunkId.ToString().GetBuf());

    if (ChunkTable.Remove(chunkId))
        return Core::Error::Success;

    return Core::Error::NotFound;
}

Core::Error Volume::ChunkLookup(const Guid& chunkId)
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

    trace(1, "Chunk %s lookup", chunkId.ToString().GetBuf());

    if (!ChunkTable.CheckExist(chunkId))
        return Core::Error::NotFound;
    return Core::Error::Success;
}

Core::Error Volume::TestJournal()
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return Core::Error::InvalidState;

    trace(1, "Test journal");

    auto tx = JournalObj.BeginTx();
    if (tx.Get() == nullptr)
    {
        return Core::Error::NoMemory;
    }

    trace(1, "Test journal, tx created %s", tx->GetTxId().ToString().GetBuf());

    unsigned long long position = (JournalObj.GetStart() + JournalObj.GetSize()) * GetBlockSize();

    Core::Error err;
    auto page = Core::Page<Core::Memory::PoolType::Kernel>::Create(err);
    if (!err.Ok())
        return err;

    err = tx->Write(*page.Get(), position);
    if (!err.Ok())
        return err;

    err = tx->Commit();

    trace(1, "Test journal, err %d", err.GetCode());

    return err;
}

}