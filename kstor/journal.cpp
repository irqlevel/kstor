#include "journal.h"
#include "api.h"
#include "volume.h"

#include <core/page.h>
#include <core/bio.h>
#include <core/bitops.h>
#include <core/xxhash.h>
#include <core/offsetof.h>
#include <core/auto_lock.h>
#include <core/shared_auto_lock.h>

namespace KStor
{

Journal::Journal(Volume& volume)
    : VolumeRef(volume)
    , Start(0)
    , Size(0)
    , State(JournalStateNew)
{
    trace(1, "Journal 0x%p ctor", this);
}

Core::Error Journal::Load(uint64_t start)
{
    Core::AutoLock lock(Lock);

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

    err = Replay();
    if (!err.Ok())
    {
        trace(0, "Journal 0x%p replay error %d", this, err.GetCode());
        return err;
    }

    TxThread = Core::MakeUnique<Core::Thread, Core::Memory::PoolType::Kernel>(this, err);
    if (TxThread.Get() == nullptr)
    {
        return Core::Error::NoMemory;
    }

    State = JournalStateRunning;
    trace(1, "Journal 0x%p start %llu size %llu", this, Start, Size);

    return Core::Error::Success;
}

Core::Error Journal::Format(uint64_t start, uint64_t size)
{
    Core::AutoLock lock(Lock);
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
    Stop();
}

Transaction::Transaction(Journal& journal, Core::Error& err)
    : JournalRef(journal)
    , State(Api::JournalTxStateNew)
{
    if (!err.Ok())
        return;
    
    err = TxId.Generate();
    if (!err.Ok())
        return;
    
    BeginBlock = CreateTxBlock(Api::JournalBlockTypeTxBegin);
    if (BeginBlock.Get() == nullptr)
    {
        err = Core::Error::NoMemory;
        return;
    }

    CommitBlock = CreateTxBlock(Api::JournalBlockTypeTxCommit);
    if (CommitBlock.Get() == nullptr)
    {
        err = Core::Error::NoMemory;
        BeginBlock.Reset();
        return;
    }
    trace(1, "Tx 0x%p %s ctor", this, TxId.ToString().GetBuf());
}

JournalTxBlockPtr Transaction::CreateTxBlock(unsigned int type)
{
    JournalTxBlockPtr block = Core::MakeShared<Api::JournalTxBlock, Core::Memory::PoolType::Kernel>();
    if (block.Get() == nullptr)
        return block;

    switch (type)
    {
    case Api::JournalBlockTypeTxBegin:
    case Api::JournalBlockTypeTxCommit:
    case Api::JournalBlockTypeTxData:
        block->TxId = TxId.GetContent();
        block->Type = type;
        break;
    default:
        block.Reset();
        break;
    }

    return block;
}

Transaction::~Transaction()
{
    trace(1, "Tx 0x%p %s dtor", this, TxId.ToString().GetBuf());
    Core::AutoLock lock(Lock);
    JournalRef.UnlinkTx(this, false);
}

Core::Error Transaction::Write(Core::Page& page, uint64_t position)
{
    Core::AutoLock lock(Lock);

    if (State != Api::JournalTxStateNew)
        return Core::Error::InvalidState;

    trace(1, "Tx 0x%p %s write %llu", this, TxId.ToString().GetBuf(), position);

    if (position < page.GetSize())
        return Core::Error::Overlap;

    if (Core::Memory::CheckIntersection(position, position + page.GetSize(),
                    JournalRef.GetStart() * JournalRef.GetBlockSize(),
                    (JournalRef.GetStart() + JournalRef.GetSize()) * JournalRef.GetBlockSize()))
        return Core::Error::Overlap;

    Core::LinkedList<JournalTxBlockPtr, Core::Memory::PoolType::Kernel> blockList;
    size_t off = 0;
    while (off < page.GetSize())
    {
        JournalTxBlockPtr blockPtr = CreateTxBlock(Api::JournalBlockTypeTxData);
        if (blockPtr.Get() == nullptr)
        {
            return Core::Error::NoMemory;
        }

        Api::JournalTxDataBlock* block = reinterpret_cast<Api::JournalTxDataBlock*>(blockPtr.Get());
        size_t read = page.Read(block->Data, sizeof(block->Data), off);
        block->Position = position;
        block->DataSize = read;

        if (!blockList.AddTail(blockPtr))
        {
            return Core::Error::NoMemory;
        }

        off += read;
        position += read;
    }

    DataBlockList.AddTail(Core::Memory::Move(blockList));

    return Core::Error::Success;
}

const Guid& Transaction::GetTxId() const
{
    return TxId;
}

Core::Error Transaction::Commit()
{
    {
        Core::AutoLock lock(Lock);

        if (State != Api::JournalTxStateNew)
            return Core::Error::InvalidState;

        State = Api::JournalTxStateCommiting;
        Core::Error err = JournalRef.StartCommitTx(this);
        if (!err.Ok())
        {
            State = Api::JournalTxStateCanceled;
            JournalRef.UnlinkTx(this, false);
            return err;
        }
    }

    CommitEvent.Wait();

    Core::AutoLock lock(Lock);
    return CommitResult;
}

void Transaction::Cancel()
{
    Core::AutoLock lock(Lock);

    State = Api::JournalTxStateCanceled;
    JournalRef.UnlinkTx(this, true);
    CommitResult = Core::Error::Cancelled;
}

TransactionPtr Journal::BeginTx()
{
    Core::SharedAutoLock lock(Lock);
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

void Journal::UnlinkTx(Transaction* tx, bool cancel)
{
    Core::SharedAutoLock lock(Lock);

    trace(1, "Journal 0x%p tx 0x%p %s unlink cancel %d",
        this, tx, tx->GetTxId().ToString().GetBuf(), cancel);

    auto txPtr = TxTable.Get(tx->GetTxId());
    if (txPtr.Get() != tx)
        return;

    TxTable.Remove(txPtr->GetTxId());
}

Core::Error Journal::StartCommitTx(Transaction* tx)
{
    Core::SharedAutoLock lock(Lock);

    auto txPtr = TxTable.Get(tx->GetTxId());
    if (txPtr.Get() != tx)
        return Core::Error::NotFound;

    {
        Core::AutoLock lock(TxListLock);
        if (!TxList.AddTail(txPtr))
            return Core::Error::NoMemory;
        TxListEvent.Set();
    }

    trace(1, "Journal 0x%p tx 0x%p %s start commit",
        this, txPtr.Get(), txPtr->GetTxId().ToString().GetBuf());

    return Core::Error::Success;
}

Core::Error Journal::FlushTx(const TransactionPtr& tx)
{
    trace(1, "Journal 0x%p tx 0x%p %s flush",
        this, tx.Get(), tx->GetTxId().ToString().GetBuf());

    return Core::Error::Success;
}

Core::Error Journal::Run(const Core::Threadable& thread)
{
    trace(1, "Journal 0x%p tx thread start", this);

    while (!thread.IsStopping())
    {
        TxListEvent.Wait(10);

        if (TxList.IsEmpty())
            continue;

        TransactionPtr tx;
        {
            Core::AutoLock lock(TxListLock);
            if (!TxList.IsEmpty())
            {
                tx = TxList.Head();
                TxList.PopHead();
            }
        }

        if (tx.Get() != nullptr)
        {
            auto err = FlushTx(tx);
            if (!err.Ok())
                tx->Cancel();
        }
    }

    trace(1, "Journal 0x%p tx thread stop", this);

    for (;;)
    {
        TransactionPtr tx;
        {
            Core::AutoLock lock(TxListLock);
            if (!TxList.IsEmpty())
            {
                tx = TxList.Head();
                TxList.PopHead();
            }
        }

        if (tx.Get() == nullptr)
            break;

        tx->Cancel();
    }

    return Core::Error::Success;
}

Core::Error Journal::Replay()
{
    Core::Error err;

    State = JournalStateReplaying;
    trace(1, "Journal 0x%p replay %d", this, err.GetCode());

    return err;
}

Core::Error Journal::ReadTxBlockComplete(Core::Page& blockPage)
{
    Core::PageMap blockMap(blockPage);
    Api::JournalTxBlock* block = reinterpret_cast<Api::JournalTxBlock*>(blockMap.GetAddress());

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(block, OFFSET_OF(Api::JournalTxBlock, Hash), hash);
    if (!Core::Memory::ArrayEqual(hash, block->Hash))
    {
        return Core::Error::DataCorrupt;
    }

    block->Type = Core::BitOps::Le32ToCpu(block->Type);
    switch (block->Type)
    {
    case Api::JournalBlockTypeTxBegin:
        break;
    case Api::JournalBlockTypeTxData:
    {
        Api::JournalTxDataBlock *dataBlock = reinterpret_cast<Api::JournalTxDataBlock*>(block);
        dataBlock->Position = Core::BitOps::Le64ToCpu(dataBlock->Position);
        dataBlock->DataSize = Core::BitOps::Le32ToCpu(dataBlock->DataSize);
        break;
    }
    case Api::JournalBlockTypeTxCommit:
    {
        Api::JournalTxCommitBlock *commitBlock = reinterpret_cast<Api::JournalTxCommitBlock*>(block);
        commitBlock->State = Core::BitOps::Le32ToCpu(commitBlock->State);
        commitBlock->Time = Core::BitOps::Le64ToCpu(commitBlock->Time);
        break;
    }
    default:
        return Core::Error::InvalidValue;
    }

    return Core::Error::Success;
}

Core::Error Journal::WriteTxBlockPrepare(Core::Page& blockPage)
{
    Core::PageMap blockMap(blockPage);
    Api::JournalTxBlock* block = reinterpret_cast<Api::JournalTxBlock*>(blockMap.GetAddress());

    switch (block->Type)
    {
    case Api::JournalBlockTypeTxBegin:
        break;
    case Api::JournalBlockTypeTxData:
    {
        Api::JournalTxDataBlock *dataBlock = reinterpret_cast<Api::JournalTxDataBlock*>(block);
        dataBlock->Position = Core::BitOps::CpuToLe64(dataBlock->Position);
        dataBlock->DataSize = Core::BitOps::CpuToLe32(dataBlock->DataSize);
        break;
    }
    case Api::JournalBlockTypeTxCommit:
    {
        Api::JournalTxCommitBlock *commitBlock = reinterpret_cast<Api::JournalTxCommitBlock*>(block);
        commitBlock->State = Core::BitOps::CpuToLe32(commitBlock->State);
        commitBlock->Time = Core::BitOps::CpuToLe64(commitBlock->Time);
        break;
    }
    default:
        return Core::Error::InvalidValue;
    }
    block->Type = Core::BitOps::CpuToLe32(block->Type);
    Core::XXHash::Sum(block, OFFSET_OF(Api::JournalTxBlock, Hash), block->Hash);

    return Core::Error::Success;
}

JournalTxBlockPtr Journal::ReadTxBlock(uint64_t index, Core::Error& err)
{
    if (index <= Start || index >= (Start + Size))
    {
        err = Core::Error::InvalidValue;
        return JournalTxBlockPtr();
    }

    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return JournalTxBlockPtr();

    err = VolumeRef.GetDevice().Read(page, index * GetBlockSize());
    if (!err.Ok())
        return JournalTxBlockPtr();

    err = ReadTxBlockComplete(page);
    if (!err.Ok())
        return JournalTxBlockPtr();

    JournalTxBlockPtr block = Core::MakeShared<Api::JournalTxBlock, Core::Memory::PoolType::Kernel>();
    if (block.Get() == nullptr)
    {
        err = Core::Error::NoMemory;
        return block;
    }

    if (page.Read(block.Get(), sizeof(*block.Get()), 0) != page.GetSize())
    {
        err = Core::Error::UnexpectedEOF;
        block.Reset();
        return block;
    }

    return block;
}

Core::Error Journal::WriteTxBlock(uint64_t index, const JournalTxBlockPtr& block)
{
    if (index <= Start || index >= (Start + Size))
        return Core::Error::InvalidValue;

    Core::Error err;
    Core::Page page(Core::Memory::PoolType::Kernel, err);
    if (!err.Ok())
        return err;

    if (page.Write(block.Get(), sizeof(*block.Get()), 0) != page.GetSize())
    {
        return Core::Error::UnexpectedEOF;
    }

    err = WriteTxBlockPrepare(page);
    if (!err.Ok())
        return err;

    return VolumeRef.GetDevice().Write(page, index * GetBlockSize());
}

size_t Journal::GetBlockSize()
{
    return VolumeRef.GetBlockSize();
}

void Journal::Stop()
{
    Core::AutoLock lock(Lock);

    trace(1, "Journal 0x%p stopping", this);

    State = JournalStateStopping;
    if (TxThread.Get() != nullptr)
    {
        TxThread->StopAndWait();
        TxThread.Reset();
    }
    State = JournalStateStopped;

    trace(1, "Journal 0x%p stopped", this);
}

}