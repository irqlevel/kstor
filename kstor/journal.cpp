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
#include <core/bug.h>

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
    if (State != JournalStateNew)
        return MakeError(Core::Error::InvalidState);

    Core::Error err;
    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;

    err = Core::BioList<>(VolumeRef.GetDevice()).SubmitWaitResult(page,
                                                    start * GetBlockSize(), false);
    if (!err.Ok())
        return err;

    Core::PageMap pageMap(*page.Get());
    Api::JournalHeader *header = static_cast<Api::JournalHeader *>(pageMap.GetAddress());
    if (Core::BitOps::Le32ToCpu(header->Magic) != Api::JournalMagic)
    {
        trace(0, "Journal 0x%p invalid header magic 0x%x", this, Core::BitOps::Le32ToCpu(header->Magic));
        return MakeError(Core::Error::BadMagic);
    }

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(header, OFFSET_OF(Api::JournalHeader, Hash), hash);
    if (!Core::Memory::ArrayEqual(hash, header->Hash))
    {
        trace(0, "Journal 0x%p invalid header hash", this);
        return MakeError(Core::Error::DataCorrupt);
    }

    uint64_t size = Core::BitOps::Le64ToCpu(header->Size);

    if (size <= 1)
        return MakeError(Core::Error::BadSize);

    uint64_t logSize = Core::BitOps::Le64ToCpu(header->LogSize);
    uint64_t logStartIndex = Core::BitOps::Le64ToCpu(header->LogStartIndex);
    uint64_t logEndIndex = Core::BitOps::Le64ToCpu(header->LogEndIndex);
    uint64_t logCapacity = Core::BitOps::Le64ToCpu(header->LogCapacity);

    trace(1, "Journal 0x%p load, logStartIndex %llu logEndIndex %llu logSize %llu logCapacity %llu",
        this, logStartIndex, logEndIndex, logSize, logCapacity);

    if (logCapacity != (size - 1))
        return MakeError(Core::Error::BadSize);

    {
        Core::AutoLock lock(LogRbLock);
        if (!LogRb.Reset(logStartIndex, logEndIndex, logSize, logCapacity))
            return MakeError(Core::Error::BadSize);
    }

    Start = start;
    Size = size;

    err = Replay();
    if (!err.Ok())
    {
        if (err == Core::Error::DataCorrupt)
        {
            trace(1, "Journal 0x%p replay error %d", this, err.GetCode());
            err = MakeError(Core::Error::Success);
        } else {
            trace(0, "Journal 0x%p replay error %d", this, err.GetCode());
            return err;
        }
    }

    Core::AString name("kstor-jrnl", err);
    if (!err.Ok())
    {
        return err;
    }

    TxThread = Core::MakeUnique<Core::Thread, Core::Memory::PoolType::Kernel>(name, this, err);
    if (TxThread.Get() == nullptr)
    {
        return MakeError(Core::Error::NoMemory);
    }

    State = JournalStateRunning;
    trace(1, "Journal 0x%p start %llu size %llu", this, Start, Size);

    return MakeError(Core::Error::Success);
}

Core::Error Journal::Format(uint64_t start, uint64_t size)
{
    Core::AutoLock lock(Lock);
    if (size <= 1)
        return MakeError(Core::Error::InvalidValue);
    if (State != JournalStateNew)
        return MakeError(Core::Error::InvalidState);

    Core::Error err;
    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return err;
    
    page->Zero();
    Core::PageMap pageMap(*page.Get());
    Api::JournalHeader *header = static_cast<Api::JournalHeader *>(pageMap.GetAddress());

    header->Magic = Core::BitOps::CpuToLe32(Api::JournalMagic);
    header->Size = Core::BitOps::CpuToLe64(size);

    header->LogSize = Core::BitOps::CpuToLe64(0);
    header->LogStartIndex = Core::BitOps::CpuToLe64(0);
    header->LogEndIndex = Core::BitOps::CpuToLe64(0);
    header->LogCapacity = Core::BitOps::CpuToLe64(size - 1);

    Core::XXHash::Sum(header, OFFSET_OF(Api::JournalHeader, Hash), header->Hash);

    trace(1, "Journal 0x%p start %llu size %llu", this, start, size);

    err = Core::BioList<>(VolumeRef.GetDevice()).SubmitWaitResult(page,
                                                        start * GetBlockSize(), true, true);
    if (!err.Ok())
    {
        trace(0, "Journal 0x%p write header err %d", this, err.GetCode());
        return err;
    }

    Start = start;
    Size = size;

    return MakeError(Core::Error::Success);
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
    Unload();
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
        err = MakeError(Core::Error::NoMemory);
        return;
    }

    CommitBlock = CreateTxBlock(Api::JournalBlockTypeTxCommit);
    if (CommitBlock.Get() == nullptr)
    {
        err = MakeError(Core::Error::NoMemory);
        BeginBlock.Reset();
        return;
    }
    trace(1, "Tx 0x%p %s ctor", this, TxId.ToString().GetConstBuf());
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
    trace(1, "Tx 0x%p %s dtor", this, TxId.ToString().GetConstBuf());
    Core::AutoLock lock(Lock);
    JournalRef.UnlinkTx(this, false);
}

Core::Error Transaction::Write(const Core::PageInterface& page, uint64_t position)
{
    Core::AutoLock lock(Lock);

    if (State != Api::JournalTxStateNew)
        return MakeError(Core::Error::InvalidState);

    trace(1, "Tx 0x%p %s write %llu data %s",
        this, TxId.ToString().GetConstBuf(), position, page.ToHex(16).GetConstBuf());

    auto err = JournalRef.CheckPosition(position, page.GetSize());
    if (!err.Ok())
        return err;

    Core::LinkedList<JournalTxBlockPtr> blockList;
    unsigned int index = DataBlockList.Count();
    size_t off = 0;
    while (off < page.GetSize())
    {
        if (position & 511)
            return MakeError(Core::Error::InvalidValue);

        auto blockPtr = CreateTxBlock(Api::JournalBlockTypeTxData);
        if (blockPtr.Get() == nullptr)
        {
            return MakeError(Core::Error::NoMemory);
        }

        Api::JournalTxDataBlock* block = reinterpret_cast<Api::JournalTxDataBlock*>(blockPtr.Get());
        size_t sizeToRead = (sizeof(block->Data) / 512) * 512;
        if (sizeToRead == 0)
            return MakeError(Core::Error::InvalidValue);

        size_t read = page.Read(block->Data, sizeToRead, off);
        block->Position = position;
        block->DataSize = read;
        block->Index = index;
        if (!blockList.AddTail(blockPtr))
        {
            return MakeError(Core::Error::NoMemory);
        }

        off += read;
        position += read;
        index++;
    }

    DataBlockList.AddTail(Core::Memory::Move(blockList));

    return MakeError(Core::Error::Success);
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
            return MakeError(Core::Error::InvalidState);

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

    Core::Error err;
    {
        Core::AutoLock lock(Lock);
        if (!CommitResult.Ok())
        {
            return CommitResult;
        }

        if (State != Api::JournalTxStateCommited)
        {
            return MakeError(Core::Error::InvalidState);
        }

        err = JournalRef.ApplyBlocks(DataBlockList, false);
        if (!err.Ok())
        {
            trace(0, "Tx 0x%p %s apply error %d", this, TxId.ToString().GetConstBuf(), err.GetCode());
        }
    }

    if (err.Ok())
    {
        err = JournalRef.EraseTx(this);
        if (!err.Ok())
        {
            trace(0, "Tx 0x%p %s erase error %d", this, TxId.ToString().GetConstBuf(), err.GetCode());
        }
    }

    //Return success despite apply/erase tx errors
    //due to tx already commited into log
    return MakeError(Core::Error::Success);
}

void Transaction::Cancel()
{
    Core::AutoLock lock(Lock);

    State = Api::JournalTxStateCanceled;
    JournalRef.UnlinkTx(this, true);
    CommitResult = MakeError(Core::Error::Cancelled);
}

Core::Error Transaction::WriteTx(Core::NoIOBioList& bioList)
{
    Core::Error err;
    Core::AutoLock lock(Lock);

    if (State != Api::JournalTxStateCommiting)
    {
        err = MakeError(Core::Error::InvalidState);
        goto fail;
    }

    size_t index;
    err = JournalRef.GetNextIndex(index);
    if (!err.Ok())
        goto fail;

    if (!IndexList.AddTail(index))
    {
        err = MakeError(Core::Error::NoMemory);
        goto fail;
    }

    err = JournalRef.WriteTxBlock(index, BeginBlock, bioList);
    if (!err.Ok())
        goto fail;

    {
        auto it = DataBlockList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            auto block = it.Get();
            err = JournalRef.GetNextIndex(index);
            if (!err.Ok())
                goto fail;

            if (!IndexList.AddTail(index))
            {
                err = MakeError(Core::Error::NoMemory);
                goto fail;
            }

            err = JournalRef.WriteTxBlock(index, block, bioList);
            if (!err.Ok())
                goto fail;
        }
    }

    err = JournalRef.GetNextIndex(index);
    if (!err.Ok())
        goto fail;

    if (!IndexList.AddTail(index))
    {
        err = MakeError(Core::Error::NoMemory);
        goto fail;
    }

    {
        Api::JournalTxCommitBlock *commitBlock = reinterpret_cast<Api::JournalTxCommitBlock*>(CommitBlock.Get());
        commitBlock->State = Api::JournalTxStateCommited;
        commitBlock->BlockCount = DataBlockList.Count();
        err = JournalRef.WriteTxBlock(index, CommitBlock, bioList);
        if (!err.Ok())
            goto fail;
    }

    return err;

fail:
    IndexList.Clear();
    OnCommitCompleteLocked(err);
    return err;
}

void Transaction::OnCommitCompleteLocked(const Core::Error& result)
{

    trace(1, "Tx 0x%p %s commit complete %d", this, TxId.ToString().GetConstBuf(), result.GetCode());

    if (!result.Ok())
    {
        State = Api::JournalTxStateCanceled;
        JournalRef.UnlinkTx(this, true);
    }
    else
    {
        State = Api::JournalTxStateCommited;
    }
    CommitResult = result;
    CommitEvent.SetAll();
}

void Transaction::OnCommitComplete(const Core::Error& result)
{
    Core::AutoLock lock(Lock);

    OnCommitCompleteLocked(result);
}

Core::LinkedList<size_t>& Transaction::GetIndexList()
{
    return IndexList;
}

void Transaction::AcquireLock()
{
    Lock.Acquire();
}

void Transaction::ReleaseLock()
{
    Lock.Release();
}

unsigned int Transaction::GetState()
{
    return State;
}

Transaction::Ptr Journal::BeginTx()
{
    Core::SharedAutoLock lock(Lock);

    if (State != JournalStateRunning)
        return Transaction::Ptr();

    Core::Error err;
    Transaction::Ptr tx = Core::MakeShared<Transaction, Core::Memory::PoolType::Kernel>(*this, err);
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
    trace(1, "Journal 0x%p tx 0x%p %s unlink cancel %d",
        this, tx, tx->GetTxId().ToString().GetConstBuf(), cancel);

    bool exist;
    auto txPtr = TxTable.Lookup(tx->GetTxId(), exist);
    if (!exist || txPtr.Get() != tx)
        return;

    TxTable.Delete(tx->GetTxId());
}

Core::Error Journal::StartCommitTx(Transaction* tx)
{
    Core::SharedAutoLock lock(Lock);

    bool exist;
    auto txPtr = TxTable.Lookup(tx->GetTxId(), exist);
    if (!exist || txPtr.Get() != tx)
        return MakeError(Core::Error::NotFound);

    {
        Core::AutoLock lock(TxListLock);
        if (!TxList.AddTail(txPtr))
            return MakeError(Core::Error::NoMemory);
        TxListEvent.SetAll();
    }

    trace(1, "Journal 0x%p tx 0x%p %s start commit",
        this, txPtr.Get(), txPtr->GetTxId().ToString().GetConstBuf());

    return MakeError(Core::Error::Success);
}

Core::Error Journal::WriteTx(const Transaction::Ptr& tx, Core::NoIOBioList& bioList)
{
    Core::AutoLock lock(Lock);
    Core::Error err;

    trace(1, "Journal 0x%p tx 0x%p %s write",
        this, tx.Get(), tx->GetTxId().ToString().GetConstBuf());

    return tx->WriteTx(bioList);
}

Core::Error Journal::Run(const Core::Threadable& thread)
{
    Core::Error err;
    trace(1, "Journal 0x%p tx thread start", this);

    Core::LinkedList<Transaction::Ptr> txList;
    while (!thread.IsStopping())
    {
        TxListEvent.Wait(10);

        if (TxList.IsEmpty())
            continue;

        {
            Core::AutoLock lock(TxListLock);
            txList = Core::Memory::Move(TxList);
        }

        Core::NoIOBioList bioList(VolumeRef.GetDevice());

        auto it = txList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            auto tx = it.Get();
            err = WriteTx(tx, bioList);
            if (!err.Ok())
                break;
        }

        if (err.Ok())
        {
            err = Flush(bioList);
        }

        while (!txList.IsEmpty())
        {
            auto tx = txList.Head();
            txList.PopHead();
            tx->OnCommitComplete(err);
        }
    }

    trace(1, "Journal 0x%p tx thread stop", this);

    return err;
}

Core::Error Journal::CheckPosition(unsigned long long position, size_t size)
{
    if (position & 511)
        return MakeError(Core::Error::InvalidValue);

    if (position < GetBlockSize())
        return MakeError(Core::Error::Overlap);

    if (Core::Memory::CheckIntersection(position, position + size,
                    GetStart() * GetBlockSize(), (GetStart() + GetSize()) * GetBlockSize()))
        return MakeError(Core::Error::Overlap);

    if (position >= VolumeRef.GetSize())
        return MakeError(Core::Error::Overflow);

    if ((position + size) > VolumeRef.GetSize())
        return MakeError(Core::Error::Overflow);

    return MakeError(Core::Error::Success);
}

Core::Error Journal::ApplyBlocks(Core::LinkedList<JournalTxBlockPtr>& blockList, bool preflushFua)
{
    Core::NoIOBioList bioList(VolumeRef.GetDevice());

    auto it = blockList.GetIterator();
    for(; it.IsValid(); it.Next())
    {
        auto block = it.Get();
        auto data = reinterpret_cast<Api::JournalTxDataBlock*>(block.Get());

        if (data->Type != Api::JournalBlockTypeTxData)
            return MakeError(Core::Error::InvalidValue);

        auto err = CheckPosition(data->Position, data->DataSize);
        if (!err.Ok())
            return err;

        trace(1, "Journal 0x%p position %llu size %lu data %s",
            this, data->Position, data->DataSize,
            Core::Hex::Encode(data->Data, 16, data->DataSize).GetConstBuf());

        err = bioList.AddIo(data->Data, data->DataSize, data->Position, true);
        if (!err.Ok())
            return err;
    }

    return bioList.SubmitWaitResult(preflushFua);
}

Core::Error Journal::Replay(Core::LinkedList<JournalTxBlockPtr>&& blockList)
{
    auto localBlockList = Core::Memory::Move(blockList);

    if (localBlockList.Count() < 3)
        return MakeError(Core::Error::DataCorrupt);

    auto beginBlock = localBlockList.Head();
    localBlockList.PopHead();
    auto commitBlock = localBlockList.Tail();
    localBlockList.PopTail();

    if (beginBlock->Type != Api::JournalBlockTypeTxBegin)
        return MakeError(Core::Error::DataCorrupt);
    if (commitBlock->Type != Api::JournalBlockTypeTxCommit)
        return MakeError(Core::Error::DataCorrupt);

    auto commitData = reinterpret_cast<Api::JournalTxCommitBlock*>(commitBlock.Get());
    if (commitData->State != Api::JournalTxStateCommited)
        return MakeError(Core::Error::DataCorrupt);

    auto txId = Guid(beginBlock->TxId);
    if (txId != Guid(commitBlock->TxId))
        return MakeError(Core::Error::DataCorrupt);

    if (commitData->BlockCount != localBlockList.Count())
        return MakeError(Core::Error::DataCorrupt);

    auto it = localBlockList.GetIterator();
    unsigned int index = 0;
    for(; it.IsValid(); it.Next())
    {
        auto block = it.Get();
        if (block->Type != Api::JournalBlockTypeTxData)
            return MakeError(Core::Error::DataCorrupt);
        if (txId != Guid(block->TxId))
            return MakeError(Core::Error::DataCorrupt);

        auto& data = *reinterpret_cast<Api::JournalTxDataBlock*>(block.Get());
        if (data.Index != index)
            return MakeError(Core::Error::DataCorrupt);

        trace(1, "Journal 0x%p tx %s block %u pos %llu size %u",
            this, txId.ToString().GetConstBuf(), data.Index, data.Position, data.DataSize);

        index++;
    }

    auto err = ApplyBlocks(localBlockList, true);

    trace(1, "Journal 0x%p tx %s replayed, err %d", this, txId.ToString().GetConstBuf(), err.GetCode());

    return err;
}

Core::Error Journal::Replay()
{
    Core::Error err;

    State = JournalStateReplaying;
    Core::AutoLock lock(LogRbLock);

    Core::LinkedList<JournalTxBlockPtr> blockList;
    for (;;)
    {
        size_t index = -1;

        err = LogRb.PopFront(index);
        if (err == Core::Error::NotFound)
        {
            err = MakeError(Core::Error::Success);
            break;
        }

        if (!err.Ok())
            break;

        auto block = ReadTxBlock(index, err);
        if (!err.Ok())
        {
            trace(0, "Journal 0x%p read index %lu err %d", this, index, err.GetCode());
            break;
        }

        trace(1, "Journal 0x%p replay index %lu block %u", this, index, block->Type);

        switch (block->Type)
        {
        case Api::JournalBlockTypeTxBegin:
        {
            if (!blockList.IsEmpty())
            {
                err = MakeError(Core::Error::DataCorrupt);
                break;
            }
            if (!blockList.AddTail(block))
            {
                err = MakeError(Core::Error::NoMemory);
                break;
            }
            break;
        }
        case Api::JournalBlockTypeTxData:
        {
            if (blockList.IsEmpty())
            {
                err = MakeError(Core::Error::DataCorrupt);
                break;
            }
            if (!blockList.AddTail(block))
            {
                err = MakeError(Core::Error::NoMemory);
                break;
            }
            break;
        }
        case Api::JournalBlockTypeTxCommit:
        {
            if (blockList.Count() < 2)
            {
                err = MakeError(Core::Error::DataCorrupt);
                break;
            }
            if (!blockList.AddTail(block))
            {
                err = MakeError(Core::Error::NoMemory);
                break;
            }

            err = Replay(Core::Memory::Move(blockList));
            break;
        }
        default:
            err = MakeError(Core::Error::DataCorrupt);
            break;
        }

        if (!err.Ok())
        {
            break;
        }
    }

    trace(1, "Journal 0x%p replay %d", this, err.GetCode());

    return err;
}

Core::Error Journal::Flush(Core::NoIOBioList& bioList)
{
    Core::AutoLock lock(Lock);

    Core::Error err;
    auto page = Core::Page<Core::Memory::PoolType::NoIO>::Create(err);
    if (!err.Ok())
        return err;
    
    page->Zero();
    Core::PageMap pageMap(*page.Get());
    Api::JournalHeader *header = static_cast<Api::JournalHeader *>(pageMap.GetAddress());


    header->Magic = Core::BitOps::CpuToLe32(Api::JournalMagic);
    header->Size = Core::BitOps::CpuToLe64(Size);
    {
        Core::SharedAutoLock lock2(LogRbLock);

        header->LogStartIndex = Core::BitOps::CpuToLe64(LogRb.GetStartIndex());
        header->LogEndIndex = Core::BitOps::CpuToLe64(LogRb.GetEndIndex());
        header->LogSize = Core::BitOps::CpuToLe64(LogRb.GetSize());
        header->LogCapacity = Core::BitOps::CpuToLe64(LogRb.GetCapacity());

        trace(1, "Journal 0x%p flush, logStartIndex %llu logEndIndex %llu logSize %llu logCapacity %llu",
            this, LogRb.GetStartIndex(), LogRb.GetEndIndex(), LogRb.GetSize(), LogRb.GetCapacity());
    }

    Core::XXHash::Sum(header, OFFSET_OF(Api::JournalHeader, Hash), header->Hash);

    err = bioList.AddIo(page, Start * GetBlockSize(), true);
    if (!err.Ok())
    {
        trace(0, "Journal 0x%p write header err %d", this, err.GetCode());
        return err;
    }

    err = bioList.SubmitWaitResult(true);
    if (!err.Ok())
    {
        trace(0, "Journal 0x%p flush err %d", this, err.GetCode());
        return err;
    }

    CompactLog();

    trace(1, "Journal 0x%p flush %d", this, err.GetCode());
    return err;
}

Core::Error Journal::ReadTxBlockComplete(Core::PageInterface& page)
{
    Api::JournalTxBlock* block;
    if (sizeof(*block) != page.GetSize())
        return MakeError(Core::Error::BadSize);

    Core::PageMap pageMap(page);
    block = reinterpret_cast<Api::JournalTxBlock*>(pageMap.GetAddress());

    unsigned char hash[Api::HashSize];
    Core::XXHash::Sum(block, OFFSET_OF(Api::JournalTxBlock, Hash), hash);
    if (!Core::Memory::ArrayEqual(hash, block->Hash))
    {
        return MakeError(Core::Error::DataCorrupt);
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
        dataBlock->Index = Core::BitOps::Le32ToCpu(dataBlock->Index);
        break;
    }
    case Api::JournalBlockTypeTxCommit:
    {
        Api::JournalTxCommitBlock *commitBlock = reinterpret_cast<Api::JournalTxCommitBlock*>(block);
        commitBlock->State = Core::BitOps::Le32ToCpu(commitBlock->State);
        commitBlock->Time = Core::BitOps::Le64ToCpu(commitBlock->Time);
        commitBlock->BlockCount = Core::BitOps::Le32ToCpu(commitBlock->BlockCount);
        break;
    }
    default:
        return MakeError(Core::Error::DataCorrupt);
    }

    return MakeError(Core::Error::Success);
}

Core::Error Journal::WriteTxBlockPrepare(Core::PageInterface& page)
{
    Api::JournalTxBlock* block;
    if (sizeof(*block) != page.GetSize())
        return MakeError(Core::Error::BadSize);

    Core::PageMap pageMap(page);
    block = reinterpret_cast<Api::JournalTxBlock*>(pageMap.GetAddress());

    switch (block->Type)
    {
    case Api::JournalBlockTypeTxBegin:
        break;
    case Api::JournalBlockTypeTxData:
    {
        Api::JournalTxDataBlock *dataBlock = reinterpret_cast<Api::JournalTxDataBlock*>(block);
        dataBlock->Position = Core::BitOps::CpuToLe64(dataBlock->Position);
        dataBlock->DataSize = Core::BitOps::CpuToLe32(dataBlock->DataSize);
        dataBlock->Index = Core::BitOps::CpuToLe32(dataBlock->Index);
        break;
    }
    case Api::JournalBlockTypeTxCommit:
    {
        Api::JournalTxCommitBlock *commitBlock = reinterpret_cast<Api::JournalTxCommitBlock*>(block);
        commitBlock->State = Core::BitOps::CpuToLe32(commitBlock->State);
        commitBlock->Time = Core::BitOps::CpuToLe64(commitBlock->Time);
        commitBlock->BlockCount = Core::BitOps::CpuToLe32(commitBlock->BlockCount);
        break;
    }
    default:
        return MakeError(Core::Error::InvalidValue);
    }
    block->Type = Core::BitOps::CpuToLe32(block->Type);
    Core::XXHash::Sum(block, OFFSET_OF(Api::JournalTxBlock, Hash), block->Hash);

    return MakeError(Core::Error::Success);
}

Core::Error Journal::IndexToPosition(size_t index, uint64_t& position)
{
    if ((index + 1) >= Size)
    {
        return MakeError(Core::Error::Overflow);
    }

    position =  (Start + 1 + index) * GetBlockSize();
    return MakeError(Core::Error::Success);
}

Core::Error Journal::PositionToIndex(uint64_t position, size_t& index)
{
    if ((position % GetBlockSize()) != 0)
        return MakeError(Core::Error::InvalidValue);

    uint64_t localIndex = position / GetBlockSize();
    if (localIndex < (Start + 1) ||
        position >= (Start + Size))
        return MakeError(Core::Error::Overflow);

    index = localIndex - (Start + 1);
    return MakeError(Core::Error::Success);
}

JournalTxBlockPtr Journal::ReadTxBlock(uint64_t index, Core::Error& err)
{
    JournalTxBlockPtr block;

    uint64_t position;
    err = IndexToPosition(index, position);
    if (!err.Ok())
        return block;

    auto page = Core::Page<>::Create(err);
    if (!err.Ok())
        return block;

    err = Core::BioList<>(VolumeRef.GetDevice()).SubmitWaitResult(page,
                                                        position, false);
    if (!err.Ok())
        return block;

    err = ReadTxBlockComplete(*page.Get());
    if (!err.Ok())
        return block;

    block = Core::MakeShared<Api::JournalTxBlock, Core::Memory::PoolType::Kernel>();
    if (block.Get() == nullptr)
    {
        err = MakeError(Core::Error::NoMemory);
        return block;
    }

    if (page->Read(block.Get(), sizeof(*block.Get()), 0) != page->GetSize())
    {
        err = MakeError(Core::Error::UnexpectedEOF);
        block.Reset();
        return block;
    }

    return block;
}

Core::Error Journal::WriteTxBlock(uint64_t index, const JournalTxBlockPtr& block, Core::NoIOBioList& bioList)
{
    uint64_t position;
    auto err = IndexToPosition(index, position);
    if (!err.Ok())
        return err;

    auto page = Core::Page<Core::Memory::PoolType::NoIO>::Create(err);
    if (!err.Ok())
        return err;

    if (page->Write(block.Get(), sizeof(*block.Get()), 0) != page->GetSize())
    {
        return MakeError(Core::Error::UnexpectedEOF);
    }

    err = WriteTxBlockPrepare(*page.Get());
    if (!err.Ok())
        return err;

    return bioList.AddIo(page, position, true);
}

size_t Journal::GetBlockSize()
{
    return VolumeRef.GetBlockSize();
}

Core::Error Journal::Unload()
{
    trace(1, "Journal 0x%p unload", this);
    {
        Core::AutoLock lock(Lock);
        if (State == JournalStateStopped)
            return MakeError(Core::Error::Success);
        if (State != JournalStateRunning)
            return MakeError(Core::Error::InvalidState);
        State = JournalStateStopping;
    }

    if (TxThread.Get() != nullptr)
    {
        TxThread->StopAndWait();
        TxThread.Reset();
    }

    TxListLock.Acquire();
    auto txList = Core::Memory::Move(TxList);
    TxListLock.Release();

    while (!txList.IsEmpty())
    {
        auto tx = txList.Head();
        txList.PopHead();
        tx->Cancel();
    }

    Core::NoIOBioList bioList(VolumeRef.GetDevice());
    auto err = Flush(bioList);

    {
        Core::AutoLock lock(Lock);
        State = JournalStateStopped;
    }

    trace(1, "Journal 0x%p unload, err %d", this, err.GetCode());

    return err;
}

Core::Error Journal::GetNextIndex(size_t& index)
{
    size_t localIndex = -1;

    Core::AutoLock lock(LogRbLock);
    auto err = LogRb.PushBack(localIndex);
    if (!err.Ok())
        return err;

    trace(1, "Journal 0x%p next index %llu", this, localIndex);
    index = localIndex;
    return MakeError(Core::Error::Success);
}

Core::Error Journal::EraseTx(Transaction* tx)
{
    Core::AutoLock lock(Lock);

    bool exist;
    auto txPtr = TxTable.Lookup(tx->GetTxId(), exist);
    if (!exist || txPtr.Get() != tx)
        return MakeError(Core::Error::NotFound);

    {
        Core::AutoLock lock(LogRbLock);
        if (!TxToErase.AddTail(txPtr))
            return MakeError(Core::Error::NoMemory);
    }

    return MakeError(Core::Error::Success);
}

void Journal::CompactLog()
{
    Core::AutoLock lock(LogRbLock);

    for (;;)
    {
        size_t count = 0;
        auto it = TxToErase.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            auto tx = it.Get();

            tx->AcquireLock();
            auto erased = LogRb.Erase(tx->GetIndexList());
            tx->ReleaseLock();

            if (erased)
            {
                trace(1, "Journal 0x%p tx %p %s erased",
                    this, tx.Get(), tx->GetTxId().ToString().GetConstBuf());
                it.Erase();
                count++;
            }
        }

        if (count == 0)
            break;
    }

    trace(1, "Journal 0x%p tx leaks %lu",
        this, TxToErase.Count());

    TxToErase.Clear();
}

}