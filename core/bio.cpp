#include "bio.h"

Bio::Bio(int pageCount, Error& err)
    : BioPtr(nullptr)
    , PageCount(pageCount)
{
    if (err != Error::Success)
    {
        return;
    }

    BioPtr = get_kapi()->alloc_bio(pageCount);
    if (!BioPtr)
    {
        err = Error::NoMemory;
        return;
    }
}

void Bio::SetBdev(BlockDevice& blockDevice)
{
    get_kapi()->set_bio_bdev(BioPtr, blockDevice.GetBdev());
}

Bio::~Bio()
{
    if (BioPtr != nullptr)
    {
        get_kapi()->free_bio(BioPtr);
    }
}