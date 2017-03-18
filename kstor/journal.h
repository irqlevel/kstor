#pragma once

#include "forwards.h"
#include "guid.h"

#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/unique_ptr.h>
#include <core/page.h>
#include <core/hash_table.h>
#include <core/rwsem.h>
#include <core/list.h>
#include <core/thread.h>
#include <core/runnable.h>
#include <core/event.h>
#include <core/bio.h>
#include <core/ring_buffer.h>
#include <core/pair.h>

namespace KStor
{

class Journal;

using JournalTxBlockPtr = Core::SharedPtr<Api::JournalTxBlock>;

class Transaction
{
friend Journal;
public:
    using Ptr = Core::SharedPtr<Transaction>;

    Transaction(Journal& journal, Core::Error &err);

    virtual ~Transaction();

    Core::Error Write(const Core::PageInterface& page, uint64_t position);

    const Guid& GetTxId() const;

    Core::Error Commit();

    void Cancel();

    void AcquireLock();
    void ReleaseLock();
    unsigned int GetState();

    Core::LinkedList<size_t>& GetIndexList();

private:

    void OnCommitCompleteLocked(const Core::Error& result);
    void OnCommitComplete(const Core::Error& result);
    JournalTxBlockPtr CreateTxBlock(unsigned int type);

    Core::Error WriteTx(Core::NoIOBioList& bioList);

    Journal& JournalRef;
    unsigned int State;
    Guid TxId;
    JournalTxBlockPtr BeginBlock;

    Core::LinkedList<JournalTxBlockPtr> DataBlockList;
    Core::LinkedList<size_t> IndexList;

    JournalTxBlockPtr CommitBlock;
    Core::RWSem Lock;
    Core::Event CommitEvent;
    Core::Error CommitResult;
};

const unsigned int JournalStateNew = 1;
const unsigned int JournalStateReplaying = 2;
const unsigned int JournalStateRunning = 3;
const unsigned int JournalStateStopping = 4;
const unsigned int JournalStateStopped = 4;

class Journal : public Core::Runnable
{

friend Transaction;

public:
    Journal(Volume& volume);

    Core::Error Load(uint64_t start);
    Core::Error Format(uint64_t start, uint64_t size);

    uint64_t GetStart();

    uint64_t GetSize();

    virtual ~Journal();

    Transaction::Ptr BeginTx();

    size_t GetBlockSize();

    Core::Error Unload();

private:
    Core::Error ApplyBlocks(Core::LinkedList<JournalTxBlockPtr>& blockList, bool preflushFua = false);

    Core::Error Replay(Core::LinkedList<JournalTxBlockPtr>&& blockList);
    Core::Error Replay();

    Core::Error StartCommitTx(Transaction* tx);
    Core::Error WriteTx(const Transaction::Ptr& tx, Core::NoIOBioList& bioList);
    void UnlinkTx(Transaction* tx, bool cancel);

    Core::Error ReadTxBlockComplete(Core::PageInterface& page);
    Core::Error WriteTxBlockPrepare(Core::PageInterface& page);

    JournalTxBlockPtr ReadTxBlock(uint64_t index, Core::Error& err);
    Core::Error WriteTxBlock(uint64_t index, const JournalTxBlockPtr& block, Core::NoIOBioList& bioList);

    Core::Error GetNextIndex(size_t& index);

    Core::Error Run(const Core::Threadable& thread) override;

    Core::Error Flush(Core::NoIOBioList& bioList);

    Core::Error CheckPosition(unsigned long long position, size_t size);

    Core::Error IndexToPosition(size_t index, uint64_t& position);

    Core::Error PositionToIndex(uint64_t position, size_t& index);

    Core::Error EraseTx(Transaction* tx);

    void CompactLog();

private:

    Volume& VolumeRef;
    Core::HashTable<Guid, Transaction::Ptr, 512> TxTable;
    Core::LinkedList<Transaction::Ptr> TxList;
    Core::UniquePtr<Core::Thread> TxThread;
    Core::RWSem TxListLock;
    Core::Event TxListEvent;
    Core::RWSem Lock;

    Core::RingBuffer LogRb;
    Core::LinkedList<Transaction::Ptr> TxToErase;
    Core::RWSem LogRbLock;

    uint64_t Start;
    uint64_t Size;
    unsigned int State;
};

}