#pragma once

#include "list.h"
#include "vector.h"
#include "error.h"
#include "bug.h"
#include "auto_lock.h"
#include "shared_auto_lock.h"
#include "rwsem.h"

namespace Core
{

template <typename K, typename V, size_t BucketCount,
          typename Lock = RWSem, Memory::PoolType PoolType = Memory::PoolType::Kernel>
class HashTable
{
public:
    HashTable()
    {
    }

    bool Insert(const K& key, const V& value)
    {
        size_t bucket = key.Hash() % BucketCount;
        AutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                return false;
            }
        }
        HashEntry entry(key, value);
        list.AddTail(Memory::Move(entry));

        return true;
    }

    bool Insert(const K& key, const V& value, Error& err)
    {
        if (!err.Ok())
            return false;

        size_t bucket = key.Hash() % BucketCount;
        AutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                return false;
            }
        }
        HashEntry entry(key, value, err);
        if (!err.Ok())
            return false;

        return list.AddTail(Memory::Move(entry));
    }

    bool Insert(K&& key, V&&value)
    {
        size_t bucket = key.Hash() % BucketCount;
        AutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                return false;
            }
        }
        HashEntry entry(Memory::Move(key), Memory::Move(value));
        return list.AddTail(Memory::Move(entry));
    }

    bool Delete(const K& key)
    {
        size_t bucket = key.Hash() % BucketCount;
        AutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                it.Erase();
                return true;
            }
        }
        return false;
    }

    V Lookup(const K& key, bool& exist)
    {
        exist = false;
        size_t bucket = key.Hash() % BucketCount;
        SharedAutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                exist = true;
                return entry.GetValue();
            }
        }
        return EmptyValue;
    }

    bool CheckExist(const K& key)
    {
        size_t bucket = key.Hash() % BucketCount;
        SharedAutoLock lock(BucketLock[bucket]);
        LinkedList<HashEntry, PoolType>& list = Bucket[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (entry.GetKey() == key)
            {
                return true;
            }
        }
        return false;
    }

    virtual ~HashTable()
    {
    }
private:
    class HashEntry
    {
    public:
        HashEntry() {}
        HashEntry(const K& key, const V& value)
            : Key(key), Value(value)
        {
        }
        HashEntry(const K& key, const V& value, Error& err)
            : Key(key, err), Value(value, err)
        {
        }
        virtual ~HashEntry() {}
        HashEntry(HashEntry&& other)
        {
            Key = Memory::Move(other.Key);
            Value = Memory::Move(other.Value);
        }

        HashEntry& operator=(HashEntry&& other)
        {
            Key = Memory::Move(other.Key);
            Value = Memory::Move(other.Value);
            return *this;
        }

        K& GetKey()
        {
            return Key;
        }

        V& GetValue()
        {
            return Value;
        }
    private:
        K Key;
        V Value;
        HashEntry(const HashEntry& other) = delete;
        HashEntry& operator=(const HashEntry& other) = delete;
    };
    LinkedList<HashEntry, PoolType> Bucket[BucketCount];
    Lock BucketLock[BucketCount];
    V EmptyValue;
};

}