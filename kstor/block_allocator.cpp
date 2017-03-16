#include "block_allocator.h"


namespace KStor
{

BlockAllocator::BlockAllocator(Volume& volume)
    : VolumeRef(volume)
{
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

}