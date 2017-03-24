#pragma once

#include "forwards.h"
#include <core/memory.h>
#include <core/error.h>
#include <core/type.h>
#include <core/page.h>
#include <core/btree.h>
#include <core/rwsem.h>
#include <core/noplock.h>
#include <core/bitmap.h>

namespace KStor
{

class BitmapBlock : public Core::BitmapInterface
{
public:
    using Ptr = Core::SharedPtr<BitmapBlock>;

    BitmapBlock(uint64_t index, Core::Error& err);
    virtual ~BitmapBlock();

    virtual Core::Error SetBit(size_t bit) override;

    virtual Core::Error ClearBit(size_t bit) override;

    virtual Core::Error TestAndSetBit(size_t bit, bool& oldValue) override;

    virtual Core::Error TestAndClearBit(size_t bit, bool& oldValue) override;

    virtual Core::Error FindSetZeroBit(size_t& bit) override;

private:
    BitmapBlock(const BitmapBlock& other) = delete;
    BitmapBlock(BitmapBlock&& other) = delete;
    BitmapBlock& operator=(const BitmapBlock& other) = delete;
    BitmapBlock& operator=(BitmapBlock&& other) = delete;

    Core::Page<>::Ptr Page;
    Core::RWSem Lock;
    uint64_t Index;
};

class BlockAllocator
{
public:
    BlockAllocator(Volume& volume);

    Core::Error Load(uint64_t start, uint64_t size);
    Core::Error Unload();

    Core::Error Alloc(uint64_t& block);
    Core::Error Free(uint64_t block);
    virtual ~BlockAllocator();
private:
    BlockAllocator(const BlockAllocator& other) = delete;
    BlockAllocator(BlockAllocator&& other) = delete;
    BlockAllocator& operator=(const BlockAllocator& other) = delete;
    BlockAllocator& operator=(BlockAllocator&& other) = delete;

    BitmapBlock::Ptr LookupBlock(uint64_t index);
    BitmapBlock::Ptr CreateBlock(uint64_t index);
    void DeleteBlock(uint64_t index);

    uint64_t Start;
    uint64_t Size;
    uint64_t MaxIndex;

    Volume& VolumeRef;
    Core::RWSem Lock;
    Core::Btree<uint64_t, BitmapBlock::Ptr, 2> BlockTree;
};

}