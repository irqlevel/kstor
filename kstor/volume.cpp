#include "volume.h"

#include <core/trace.h>
#include <core/bio.h>
#include <core/bitops.h>
#include <core/hex.h>

namespace KStor
{

Volume::Volume(const Core::AString& deviceName, bool format, Core::Error& err)
    : DeviceName(deviceName, err)
    , Device(DeviceName, err)
    , HeaderPage(Core::Memory::PoolType::Kernel, err)
{
    if (!err.Ok())
    {
        goto out;
    }

    trace(1, "Volume 0x%p size 0x%llx", this, Device.GetSize());

    HeaderPage.Zero();

    if (format)
    {
        err = Format();
    }

    if (!err.Ok())
    {
        goto out;
    }

    err = Load();

out:
    trace(1, "Volume 0x%p name %s ctor err %d", this, deviceName.GetBuf(), err.GetCode());
}

Volume::~Volume()
{
    trace(1, "Volume 0x%p dtor", this);
}

Core::Error Volume::Format()
{
    VolumeId.Generate();

    Core::PageMap headerMap(HeaderPage);
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(headerMap.GetAddress());
    header->Magic = Core::BitOps::CpuToLe32(Api::VolumeMagic);
    header->VolumeId = VolumeId.GetContent();

    Core::Error err;
    Core::Bio bio(Device, HeaderPage, 0, err, true);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p can't init bio, err %d", this, err.GetCode());
        return err;
    }
    bio.Submit();
    bio.Wait();
    err = bio.GetError();

    trace(1, "Volume 0x%p format err %d", this, err.GetCode());

    return err;
}

Core::Error Volume::Load()
{
    Core::Error err;
    Core::Bio bio(Device, HeaderPage, 0, err, false);
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p can't init bio, err %d", this, err.GetCode());
        return err;
    }
    bio.Submit();
    bio.Wait();
    err = bio.GetError();
    if (!err.Ok())
    {
        trace(0, "Volume 0x%p bio read header, err %d", this, err.GetCode());
        return err;
    }

    Core::PageMap headerMap(HeaderPage);
    Api::VolumeHeader *header = static_cast<Api::VolumeHeader*>(headerMap.GetAddress());
    if (Core::BitOps::Le32ToCpu(header->Magic) != Api::VolumeMagic)
    {
        trace(0, "Volume 0x%p bad header magic", this);
        return Core::Error::BadMagic;
    }

    VolumeId.SetContent(header->VolumeId);

    trace(1, "Volume 0x%p load volumeId %s", this, VolumeId.ToString().GetBuf());

    return err;
}

const Guid& Volume::GetVolumeId() const
{
    return VolumeId;
}

unsigned long long Volume::GetSize() const
{
    return Device.GetSize();
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