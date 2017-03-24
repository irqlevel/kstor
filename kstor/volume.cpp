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
    , TxJournal(*this)
    , Balloc(*this)
    , State(VolumeStateNew)
{
    if (!err.Ok())
    {
        return;
    }

    trace(1, "Volume 0x%p name %s size %llu ctor",
        this, deviceName.GetConstBuf(), Device.GetSize());
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
        return MakeError(Core::Error::InvalidState);

    uint64_t size = Device.GetSize();
    if (size == 0 || size % BlockSize)
        return MakeError(Core::Error::InvalidValue);

    Size = size;

    Core::Error err = TxJournal.Format(1, (size / 10) / BlockSize);
    if (!err.Ok())
        return err;

    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;
    page->Zero();

    VolumeId.Generate();
    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    header->Magic = Core::BitOps::CpuToLe32(Api::VolumeMagic);
    header->VolumeId = VolumeId.GetContent();
    header->Size = Core::BitOps::CpuToLe64(size);
    header->JournalSize = Core::BitOps::CpuToLe64(TxJournal.GetSize());

    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), header->Hash);

    err = Core::BioList<>(Device).SubmitWaitResult(page, 0, true, true);

    trace(1, "Volume 0x%p write header, err %d", this, err.GetCode());

    return err;
}

Core::Error Volume::Load()
{
    Core::AutoLock lock(Lock);
    if (State != VolumeStateNew)
        return MakeError(Core::Error::InvalidState);

    Core::Error err;
    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;

    err = Core::BioList<>(Device).SubmitWaitResult(page, 0, false);
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
        return MakeError(Core::Error::BadMagic);
    }

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), hash);

    if (!Core::Memory::ArrayEqual(header->Hash, hash))
    {
        trace(0, "Volume 0x%p bad header hash", this);
        return MakeError(Core::Error::DataCorrupt);
    }

    uint64_t size = Core::BitOps::Le64ToCpu(header->Size);
    if (size % BlockSize)
    {
        trace(0, "Volume 0x%p bad size %llu", this, size);
        return MakeError(Core::Error::BadSize);
    }

    if (size == 0 || size != Device.GetSize())
    {
        trace(0, "Volume 0x%p bad size %llu", this, size);
        return MakeError(Core::Error::BadSize);
    }
    Size = size;

    uint64_t journalSize = Core::BitOps::Le64ToCpu(header->JournalSize);
    err = TxJournal.Load(1);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p can't load journal, err %d", this, err.GetCode());
        return err;
    }

    if (TxJournal.GetSize() != journalSize)
    {
        trace(0, "Volume 0x%p bad journal size %llu vs. %llu",
            this, TxJournal.GetSize(), journalSize);
        return MakeError(Core::Error::BadSize);
    }

    VolumeId.SetContent(header->VolumeId);

    State = VolumeStateRunning;
    trace(1, "Volume 0x%p load volumeId %s size %llu blockSize %llu",
        this, VolumeId.ToString().GetConstBuf(), Size, BlockSize);

    return err;
}

Core::Error Volume::Unload()
{
    trace(1, "Volume 0x%p unload", this);

    Core::AutoLock lock(Lock);
    if (State == VolumeStateStopped)
        return MakeError(Core::Error::Success);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    State = VolumeStateStopping;
    auto err = TxJournal.Unload();
    if (!err.Ok())
        return err;

    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;

    page->Zero();

    Core::PageMap pageMap(*page.Get());
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(pageMap.GetAddress());
    header->Magic = Core::BitOps::CpuToLe32(Api::VolumeMagic);
    header->VolumeId = VolumeId.GetContent();
    header->Size = Core::BitOps::CpuToLe64(Size);
    header->JournalSize = Core::BitOps::CpuToLe64(TxJournal.GetSize());

    Core::XXHash::Sum(header, OFFSET_OF(Api::VolumeHeader, Hash), header->Hash);

    err = Core::BioList<>(Device).SubmitWaitResult(page, 0, true, true);

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
        return MakeError(Core::Error::InvalidState);

    trace(1, "Chunk %s create", chunkId.ToString().GetConstBuf());

    bool exist;
    auto chunk = ChunkTable.Lookup(chunkId, exist);
    if (exist)
    {
        return MakeError(Core::Error::AlreadyExists);
    }

    chunk = Core::MakeShared<Chunk, Core::Memory::PoolType::Kernel>(chunkId);
    if (chunk.Get() == nullptr)
        return MakeError(Core::Error::NoMemory);

    if (!ChunkTable.Insert(chunk->ChunkId, chunk))
        return MakeError(Core::Error::NoMemory);

    return MakeError(Core::Error::Success);
}

Core::Error Volume::ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Chunk %s write", chunkId.ToString().GetConstBuf());

    bool exist;
    auto chunk = ChunkTable.Lookup(chunkId, exist);
    if (!exist)
    {
        return MakeError(Core::Error::NotFound);
    }

    Core::Memory::MemCpy(chunk->Data, data, sizeof(chunk->Data));

    trace(3, "Chunk %s write %s size %lu", chunkId.ToString().GetConstBuf(),
        Core::Hex::Encode(chunk->Data, 10).GetConstBuf(), sizeof(chunk->Data));

    return MakeError(Core::Error::Success);
}

Core::Error Volume::ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Chunk %s read", chunkId.ToString().GetConstBuf());

    bool exist;
    auto chunk = ChunkTable.Lookup(chunkId, exist);
    if (!exist)
    {
        return MakeError(Core::Error::NotFound);
    }

    Core::Memory::MemCpy(data, chunk->Data, sizeof(chunk->Data));

    trace(3, "Chunk %s read %s size %lu", chunkId.ToString().GetConstBuf(),
        Core::Hex::Encode(data, 10).GetConstBuf(), sizeof(chunk->Data));

    return MakeError(Core::Error::Success);
}

Core::Error Volume::ChunkDelete(const Guid& chunkId)
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Chunk %s delete", chunkId.ToString().GetConstBuf());

    if (ChunkTable.Delete(chunkId))
        return MakeError(Core::Error::Success);

    return MakeError(Core::Error::NotFound);
}

Core::Error Volume::ChunkLookup(const Guid& chunkId)
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Chunk %s lookup", chunkId.ToString().GetConstBuf());

    if (!ChunkTable.CheckExist(chunkId))
        return MakeError(Core::Error::NotFound);
    return MakeError(Core::Error::Success);
}

Core::Error Volume::TestJournal()
{
    Core::SharedAutoLock lock(Lock);
    if (State != VolumeStateRunning)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Test journal");

    auto tx = TxJournal.BeginTx();
    if (tx.Get() == nullptr)
    {
        return MakeError(Core::Error::NoMemory);
    }

    trace(1, "Test journal, tx created %s", tx->GetTxId().ToString().GetConstBuf());

    unsigned long long position = (TxJournal.GetStart() + TxJournal.GetSize()) * GetBlockSize();

    Core::Error err;
    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;

    for (int i = 0; i < 2; i++)
    {
        page->FillRandom();

        err = tx->Write(*page.Get(), position);
        if (!err.Ok())
            return err;

        position+= page->GetSize();
    }

    err = tx->Commit();

    trace(1, "Test journal, err %d", err.GetCode());

    return err;
}

}