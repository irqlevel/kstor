#pragma once

#include "forwards.h"
#include <core/memory.h>
#include <core/error.h>
#include <core/type.h>
#include <core/page.h>
#include <core/btree.h>
#include <core/rwsem.h>

namespace KStor
{

class BitmapBlock
{
public:
    using Ptr = Core::SharedPtr<BitmapBlock>;

    BitmapBlock(uint64_t index, Core::Error& err);
    virtual ~BitmapBlock();
private:
    BitmapBlock(const BitmapBlock& other) = delete;
    BitmapBlock(BitmapBlock&& other) = delete;
    BitmapBlock& operator=(const BitmapBlock& other) = delete;
    BitmapBlock& operator=(BitmapBlock&& other) = delete;

    Core::Page<>::Ptr Page;
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

    Volume& VolumeRef;
    uint64_t Start;
    uint64_t Size;
    uint64_t MaxIndex;

    Core::Btree<uint64_t, BitmapBlock::Ptr, 2> BlockTree;
};

}