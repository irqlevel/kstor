#pragma once

#include "list_entry.h"
#include "offsetof.h"
#include "bug.h"
#include "new.h"

namespace Core
{

template <typename T, Memory::PoolType PoolType = Memory::PoolType::Kernel> class LinkedList
{
public:

    class Iterator
    {
    public:
        Iterator()
            : CurrListEntry(nullptr)
            , EndList(nullptr)
        {
        }

        Iterator(const Iterator& other)
        {
            CurrListEntry = other.CurrListEntry;
            EndList = other.EndList;
        }

        Iterator(LinkedList& List)
            : Iterator()
        {
            CurrListEntry = List.ListHead.Flink;
            EndList = &List.ListHead;
        }

        Iterator& operator=(const Iterator& other)
        {
            if (this != &other)
            {
                CurrListEntry = other.CurrListEntry;
                EndList = other.EndList;
            }
            return *this;
        }

        Iterator& operator=(Iterator&& other)
        {
            if (this != &other)
            {
                CurrListEntry = other.CurrListEntry;
                EndList = other.EndList;
                other.CurrListEntry = nullptr;
                other.EndList = nullptr; 
            }
            return *this;
        }

        T& Get()
        {
            panic(CurrListEntry == EndList);
            LinkedListNode* node = CONTAINING_RECORD(CurrListEntry,
                                                     LinkedListNode,
                                                     ListLink);
            return node->Value;
        }

        bool IsValid()
        {
            if (CurrListEntry != nullptr && EndList != nullptr)
            {
                return (CurrListEntry != EndList) ? true : false;
            }
            else
            {
                return false;
            }
        }

        void Next()
        {
            if (IsValid())
            {
                CurrListEntry = CurrListEntry->Flink;
            }
        }

        void Erase()
        {
            panic(!IsValid());

            ListEntry* next = CurrListEntry->Flink;

            LinkedListNode* node = CONTAINING_RECORD(CurrListEntry,
                                                     LinkedListNode,
                                                     ListLink);
            RemoveEntryList(&node->ListLink);
            delete node;
            CurrListEntry = next;
        }

        virtual ~Iterator()
        {

        }
    private:
        ListEntry* CurrListEntry;
        ListEntry* EndList;
    };

    LinkedList()
    {
        InitializeListHead(&ListHead);
    }

    bool AddHead(const T& value)
    {
        LinkedListNode* node = new (PoolType) LinkedListNode(value);

        if (!node)
        {
            return false;
        }
        InsertHeadList(&ListHead, &node->ListLink);
        return true;
    }

    bool AddTail(const T& value)
    {
        LinkedListNode* node = new (PoolType) LinkedListNode(value);

        if (!node)
        {
            return false;
        }
        InsertTailList(&ListHead, &node->ListLink);
        return true;
    }

    bool AddTail(T&& value)
    {
        LinkedListNode* node = new (PoolType)
                                LinkedListNode(Memory::Move(value));
        if (!node)
        {
            return false;
        }
        InsertTailList(&ListHead, &node->ListLink);
        return true;
    }

    void AddTail(LinkedList&& other)
    {
        if (IsListEmpty(&other.ListHead))
            return;

        ListEntry* entry = other.ListHead.Flink;
        RemoveInitEntryList(&other.ListHead);
        AppendTailList(&ListHead, entry);
        return;
    }

    T& Head()
    {
        LinkedListNode* node;

        panic(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(ListHead.Flink, LinkedListNode, ListLink);
        return node->Value;
    }

    T& Tail()
    {
        LinkedListNode* node;

        panic(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(ListHead.Blink, LinkedListNode, ListLink);
        return node->Value;
    }

    void PopHead()
    {
        LinkedListNode* node;

        panic(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(RemoveHeadList(&ListHead),
                                 LinkedListNode, ListLink);
        delete node;
    }

    void PopTail()
    {
        LinkedListNode* node;

        panic(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(RemoveTailList(&ListHead),
                                 LinkedListNode, ListLink);
        delete node;
    }

    bool IsEmpty()
    {
        return (IsListEmpty(&ListHead)) ? true : false;
    }

    virtual ~LinkedList()
    {
        Release();
    }

    LinkedList(LinkedList&& other)
    {
        InitializeListHead(&ListHead);
        AddTail(Memory::Move(other));
    }

    LinkedList& operator=(LinkedList&& other)
    {
        if (this != &other)
        {
            Release();

            InitializeListHead(&ListHead);
            AddTail(Memory::Move(other));
        }
        return *this;
    }

    Iterator GetIterator()
    {
        return Iterator(*this);
    }

    size_t Count()
    {
        size_t count = 0;
        auto it = GetIterator();
        for (; it.IsValid();it.Next())
        {
            count++;
        }
        return count;
    }

    void Clear()
    {
        Release();
    }

private:
    LinkedList(const LinkedList& other) = delete;
    LinkedList& operator=(const LinkedList& other) = delete;

    void Release()
    {
        LinkedListNode* node;
        while (!IsListEmpty(&ListHead))
        {
            node = CONTAINING_RECORD(RemoveHeadList(&ListHead),
                                     LinkedListNode, ListLink);
            delete node;
        }
    }
    class LinkedListNode
    {
    public:
        LinkedListNode(const T& value)
        {
            InitializeListHead(&ListLink);
            Value = value;
        }
        LinkedListNode(T&& value)
        {
            InitializeListHead(&ListLink);
            Value = Memory::Move(value);
        }
        virtual ~LinkedListNode()
        {
        }
        ListEntry ListLink;
        T Value;
    private:
        LinkedListNode() = delete;
        LinkedListNode(const LinkedListNode& other) = delete;
        LinkedListNode& operator=(const LinkedListNode& other) = delete;
        LinkedListNode& operator=(LinkedListNode&& other) = delete;
    };
    ListEntry ListHead;
};

}