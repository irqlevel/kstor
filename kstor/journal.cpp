#include "journal.h"
#include "api.h"
#include "volume.h"

#include <core/page.h>
#include <core/bio.h>
#include <core/bitops.h>
#include <core/xxhash.h>
#include <core/offsetof.h>

namespace KStor
{

Journal::Journal(Volume& volume)
    : VolumeRef(volume), Start(0), Size(0)
{
    trace(1, "Journal 0x%p ctor", this);
}

Core::Error Journal::Load(uint64_t start)
{
    Core::Error err;
    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return err;

    err =  VolumeRef.GetDevice().Read(page, start * VolumeRef.GetBlockSize());
    if (!err.Ok())
        return err;

    Core::PageMap pageMap(page);
    Api::JournalHeader *header = static_cast<Api::JournalHeader *>(pageMap.GetAddress());
    if (Core::BitOps::Le32ToCpu(header->Magic) != Api::JournalMagic)
    {
        trace(0, "Journal 0x%p invalid header magic", this);
        return Core::Error::BadMagic;
    }

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(header, OFFSET_OF(Api::JournalHeader, Hash), hash);
    if (!Core::Memory::ArrayEqual(hash, header->Hash))
    {
        trace(0, "Journal 0x%p invalid header hash", this);
        return Core::Error::DataCorrupt;       
    }

    uint64_t size = Core::BitOps::Le64ToCpu(header->Size);

    if (size <= 1)
        return Core::Error::BadSize;

    Start = start;
    Size = size;

    trace(1, "Journal 0x%p start %llu size %llu", this, Start, Size);
    return Core::Error::Success;
}

Core::Error Journal::Format(uint64_t start, uint64_t size)
{
    if (size <= 1)
        return Core::Error::InvalidValue;

    Core::Error err;
    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return err;
    
    page.Zero();
    Core::PageMap pageMap(page);
    Api::JournalHeader *header = static_cast<Api::JournalHeader *>(pageMap.GetAddress());

    header->Magic = Core::BitOps::CpuToLe32(Api::JournalMagic);
    header->Size = Core::BitOps::CpuToLe64(size);
    Core::XXHash::Sum(header, OFFSET_OF(Api::JournalHeader, Hash), header->Hash);

    trace(1, "Journal 0x%p start %llu size %llu", this, start, size);

    err =  VolumeRef.GetDevice().Write(page, start * VolumeRef.GetBlockSize());
    if (!err.Ok())
    {
        trace(0, "Journal 0x%p write header err %d", this, err.GetCode());
        return err;
    }

    Start = start;
    Size = size;

    return Core::Error::Success;
}

uint64_t Journal::GetStart()
{
    return Start;
}

uint64_t Journal::GetSize()
{
    return Size;
}

Journal::~Journal()
{
    trace(1, "Journal 0x%p dtor", this);
}

Transaction::Transaction(Journal& journal, Core::Error& err)
    : JournalRef(journal)
{
    if (!err.Ok())
        return;
    
    err = TxId.Generate();
    if (!err.Ok())
        return;
    
    trace(1, "Tx 0x%p %s ctor", this, TxId.ToString().GetBuf());
}

Transaction::~Transaction()
{
    trace(1, "Tx 0x%p %s dtor", this, TxId.ToString().GetBuf());
}

Core::Error Transaction::Write(Core::Page& page, uint64_t position)
{
    Core::Error err;

    trace(1, "Tx 0x%p %s write %llu", this, TxId.ToString().GetBuf(), position);

    return err;
}

const Guid& Transaction::GetTxId() const
{
    return TxId;
}

TransactionPtr Journal::BeginTx()
{
    Core::Error err;
    TransactionPtr tx = Core::MakeShared<Transaction, Core::Memory::PoolType::Kernel>(*this, err);
    if (tx.Get() == nullptr)
    {
        return tx;
    }

    if (!err.Ok())
    {
        tx.Reset();
        return tx;
    }

    if (!TxTable.Insert(tx->GetTxId(), tx))
    {
        tx.Reset();
        return tx;
    }

    return tx;
}

Core::Error Journal::CommitTx(const TransactionPtr& tx)
{
    Core::Error err;

    TxTable.Remove(tx->GetTxId());

    trace(1, "Tx 0x%p %s commit %d", this, tx->GetTxId().ToString().GetBuf(), err);
    return err;
}

Core::Error Journal::Replay()
{
    Core::Error err;

    trace(1, "Journal 0x%p replay %d", this, err.GetCode());

    return err;
}


JournalBlockPtr Journal::ReadBlock(uint64_t index, Core::Error& err)
{
    if (index <= Start || index >= (Start + Size))
    {
        err = Core::Error::InvalidValue;
        return JournalBlockPtr();
    }

    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return JournalBlockPtr();

    err = VolumeRef.GetDevice().Read(page, index * VolumeRef.GetBlockSize());
    if (!err.Ok())
        return JournalBlockPtr();

    JournalBlockPtr block = Core::MakeShared<Api::JournalBlock, Core::Memory::PoolType::Kernel>();
    if (block.Get() == nullptr)
    {
        err = Core::Error::NoMemory;
        return block;
    }

    page.Read(block.Get(), sizeof(*block.Get()));

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(block.Get(), OFFSET_OF(Api::JournalBlock, Hash), hash);
    if (!Core::Memory::ArrayEqual(hash, block->Hash))
    {
        err = Core::Error::DataCorrupt;
        trace(0, "Journal 0x%p block %llu data corrupt", this, index);
        block.Reset();
        return block;
    }

    return block;
}

Core::Error Journal::WriteBlock(uint64_t index, const JournalBlockPtr& block)
{
    if (index <= Start || index >= (Start + Size))
        return Core::Error::InvalidValue;

    Core::Error err;
    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return err;

    Core::XXHash::Sum(block.Get(), OFFSET_OF(Api::JournalBlock, Hash), block->Hash);
    page.Write(block.Get(), sizeof(*block.Get()));
    return VolumeRef.GetDevice().Write(page, index * VolumeRef.GetBlockSize());
}

}