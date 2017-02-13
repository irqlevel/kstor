#pragma once

#include "forwards.h"
#include "guid.h"

#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/page.h>
#include <core/hash_table.h>
#include <core/rwsem.h>

namespace KStor
{

class Journal;

class Transaction
{
public:
    Transaction(Journal& journal, Core::Error &err);

    virtual ~Transaction();

    Core::Error Write(Core::Page& page, uint64_t position);

    const Guid& GetTxId() const;

private:
    Journal& JournalRef;
    Guid TxId;
};

typedef Core::SharedPtr<Transaction, Core::Memory::PoolType::Kernel> TransactionPtr;

typedef Core::SharedPtr<Api::JournalBlock, Core::Memory::PoolType::Kernel> JournalBlockPtr;

class Journal
{
public:
    Journal(Volume& volume);

    Core::Error Load(uint64_t start);

    Core::Error Format(uint64_t start, uint64_t size);

    uint64_t GetStart();

    uint64_t GetSize();

    virtual ~Journal();

    TransactionPtr BeginTx();
    
    Core::Error CommitTx(const TransactionPtr& tx);

    Core::Error Replay();

private:

    JournalBlockPtr ReadBlock(uint64_t index, Core::Error& err);
    Core::Error WriteBlock(uint64_t index, const JournalBlockPtr& block);

private:

    Volume& VolumeRef;
    Core::HashTable<Guid, TransactionPtr, Core::RWSem, Core::Memory::PoolType::Kernel, 512> TxTable;
    uint64_t Start;
    uint64_t Size;
};

}