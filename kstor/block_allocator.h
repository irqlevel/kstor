#pragma once

#include "forwards.h"
#include <core/memory.h>
#include <core/error.h>
#include <core/type.h>

namespace KStor
{

class BlockAllocator
{
public:
    BlockAllocator(Volume& volume);
    Core::Error Alloc(uint64_t& block);
    Core::Error Free(uint64_t block);
    virtual ~BlockAllocator();
private:
    BlockAllocator(const BlockAllocator& other) = delete;
    BlockAllocator(BlockAllocator&& other) = delete;
    BlockAllocator& operator=(const BlockAllocator& other) = delete;
    BlockAllocator& operator=(BlockAllocator&& other) = delete;

    Volume& VolumeRef;
};

}