#include "block_allocator.h"
#include "volume.h"

namespace KStor
{

BitmapBlock::BitmapBlock(uint64_t index, Core::Error& err)
    : Index(index)
{
    if (!err.Ok())
        return;

    Page = Core::Page<>::Create(err);
    if (!err.Ok())
        return;
}

BitmapBlock::~BitmapBlock()
{
}

BlockAllocator::BlockAllocator(Volume& volume)
    : VolumeRef(volume)
{
}

Core::Error BlockAllocator::Load(uint64_t start, uint64_t size)
{
    if (start % VolumeRef.GetBlockSize())
    {
        return Core::Error::InvalidValue;
    }

    if (size % VolumeRef.GetBlockSize())
    {
        return Core::Error::InvalidValue;
    }

    if ((start + size) > VolumeRef.GetSize())
    {
        return Core::Error::InvalidValue;
    }

    uint64_t maxIndex = start / size;
    if (maxIndex == 0)
    {
        return Core::Error::InvalidValue;
    }

    Start = start;
    Size = size;
    MaxIndex = maxIndex;

    return Core::Error::Success;
}

Core::Error BlockAllocator::Unload()
{
    return Core::Error::Success;
}

BlockAllocator::~BlockAllocator()
{
}

Core::Error Alloc(uint64_t& block)
{
    return Core::Error::NotImplemented;
}

Core::Error Free(uint64_t block)
{
    return Core::Error::NotImplemented;
}

BitmapBlock::Ptr BlockAllocator::LookupBlock(uint64_t index)
{
    Core::SharedAutoLock lock(Lock);

    bool exist;
    auto block = BlockTree.Lookup(index, exist);
    if (!exist)
    {
        block.Reset();
    }

    return block;
}

BitmapBlock::Ptr BlockAllocator::CreateBlock(uint64_t index)
{
    {
        Core::SharedAutoLock lock(Lock);
        bool exist;
        auto block = BlockTree.Lookup(index, exist);
        if (exist)
        {
            return block;
        }
    }

    Core::AutoLock lock(Lock);
    Core::Error err;
    auto block = Core::MakeShared<BitmapBlock, Core::Memory::PoolType::Kernel>(index, err);
    if (block.Get() == nullptr)
        return block;

    if (!err.Ok())
    {
        block.Reset();
        return block;
    }

    if (!BlockTree.Insert(index, block))
    {
        block.Reset();

        bool exist;
        block = BlockTree.Lookup(index, exist);
        if (!exist)
        {
            block.Reset();
        }
    }

    return block;
}

void BlockAllocator::DeleteBlock(uint64_t index)
{
    Core::AutoLock lock(Lock);

    BlockTree.Delete(index);
}

}