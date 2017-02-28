#include "bio.h"
#include "trace.h"

namespace Core
{

/*
BioList::BioList(BlockDevice& blockDevice)
    : BlockDev(blockDevice)
{
}

BioList::~BioList()
{
}

Error BioList::AddIo(Page& page, unsigned long long position, bool write)
{
    if (position & 511)
        return Error::InvalidValue;

    Error err;
    BioPtr bio = MakeShared<Bio, Memory::PoolType::Kernel>(BlockDev, page, position / 512, err, write);
    if (bio.Get() == nullptr)
    {
        return Error::NoMemory;
    }

    if (!err.Ok())
    {
        return err;
    }

    if (!ReqList.AddTail(bio))
    {
        return Error::NoMemory;
    }

    return Error::Success;
}

void BioList::Submit(bool flushFua)
{
    if (ReqList.IsEmpty())
        return;

    if (flushFua)
    {
        auto lastBio = ReqList.Tail();
        lastBio->SetFlush();
        lastBio->SetFua();
    }

    auto it = ReqList.GetIterator();
    for (;it.IsValid(); it.Next())
    {
        ReqCompleteCount.Inc();
    }

    it = ReqList.GetIterator();
    for (;it.IsValid(); it.Next())
    {
        auto bio = it.Get();
        bio->Submit();
    }
}
*/
}