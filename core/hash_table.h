#pragma once

#include "list.h"
#include "vector.h"
#include "error.h"
#include "bug.h"

namespace Core
{

template <class K, class V, Memory::PoolType PoolType>
class HashTable
{
public:
    HashTable(size_t nrBuckets, Error& err,
              int (*keyCmp)(const K& key1, const K& key2),
              size_t (*keyHash)(const K& key))
        : Buckets(), KeyCmp(keyCmp), KeyHash(keyHash)
    {
        if (err != Error::Success)
            return;

        if (!Buckets.Reserve(nrBuckets))
        {
            err = Error::NoMemory;
            return;
        }

        for (size_t i = 0; i < nrBuckets; i++)
        {
            LinkedList<HashEntry, PoolType> list;
            if (!Buckets.PushBack(Memory::Move(list)))
            {
                err = Error::NoMemory;
                return;
            }
        }
    }

    bool Insert(const K& key, const V& value)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
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
        if (err != Error::Success)
            return false;

        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return false;
            }
        }
        HashEntry entry(key, value, err);
        if (err != Error::Success)
            return false;

        list.AddTail(Memory::Move(entry));
        return true;
    }

    bool Insert(K&& key, V&&value)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return false;
            }
        }
        HashEntry entry(Memory::Move(key), Memory::Move(value));
        list.AddTail(Memory::Move(entry));

        return true;
    }


    bool Remove(const K& key)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                it.Erase();
                return true;
            }
        }
        return false;
    }

    V& Get(const K& key)
    {
        BugOn(!Exists(key));

        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
            {
                return entry.GetValue();
            }
        }
        return EmptyValue;
    }

    bool Exists(const K& key)
    {
        size_t bucket = KeyHash(key) % Buckets.GetSize();
        LinkedList<HashEntry, PoolType>& list = Buckets[bucket];
        auto it = list.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            HashEntry& entry = it.Get();
            if (KeyCmp(entry.GetKey(), key) == 0)
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
    Vector<LinkedList<HashEntry, PoolType>, PoolType> Buckets;
    int (*KeyCmp)(const K& key1, const K& key2);
    size_t (*KeyHash)(const K& key);
    V EmptyValue;
};

}