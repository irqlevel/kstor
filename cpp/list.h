#pragma once

#include "main.h"
#include "list_entry.h"

template <class T> class LinkedList
{
public:

    template <class T> class Iterator
    {
    public:
        Iterator(LinkedList<T>& List)
        {
            CurrListEntry = List.ListHead.Flink;
            EndList = &List.ListHead;
        }
        T Get()
        {
            KBUG_ON(CurrListEntry == EndList);
            LinkedListNode<T>* node = CONTAINING_RECORD(CurrListEntry,
                                                        LinkedListNode<T>,
                                                        ListEntry);
            return node->Value;
        }

        bool IsValid()
        {
            return (CurrListEntry != EndList) ? true : false;
        }

        void Next()
        {
            if (IsValid())
            {
                CurrListEntry = CurrListEntry->Flink;
            }
        }

        virtual ~Iterator()
        {

        }
    private:
        PLIST_ENTRY CurrListEntry;
        PLIST_ENTRY EndList;
    };

    LinkedList()
    {
        InitializeListHead(&ListHead);
    }
    bool AddHead(T value)
    {
        LinkedListNode<T>* node = new LinkedListNode<T>(value);

        if (!node)
        {
            return false;
        }
        InsertHeadList(&ListHead, &node->ListEntry);
        return true;
    }

    bool AddTail(T value)
    {
        LinkedListNode<T>* node = new LinkedListNode<T>(value);

        if (!node)
        {
            return false;
        }
        InsertTailList(&ListHead, &node->ListEntry);
        return true;
    }

    T PopHead()
    {
        LinkedListNode<T>* node;

        KBUG_ON(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(RemoveHeadList(&ListHead), LinkedListNode<T>,
                                 ListEntry);
        T value = node->Value;
        delete node;
        return value;
    }

    T PopTail()
    {
        LinkedListNode<T>* node;

        KBUG_ON(IsListEmpty(&ListHead));
        node = CONTAINING_RECORD(RemoveTailList(&ListHead), LinkedListNode<T>,
                                 ListEntry);
        T value = node->Value;
        delete node;
        return value;
    }

    bool IsEmpty()
    {
        return (IsListEmpty(&ListHead)) ? true : false;
    }

    virtual ~LinkedList()
    {
        LinkedListNode<T>* node;
        while (!IsListEmpty(&ListHead))
        {
            node = CONTAINING_RECORD(RemoveHeadList(&ListHead),
                                     LinkedListNode<T>, ListEntry);
            delete node;
        }
    }
private:
    template <class T> class LinkedListNode
    {
    public:
        LinkedListNode(T value)
        {
            InitializeListHead(&ListEntry);
            Value = value;
        }
        virtual ~LinkedListNode()
        {
        }
        LIST_ENTRY ListEntry;
        T Value;
    };
    LIST_ENTRY ListHead;
};
