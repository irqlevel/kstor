#pragma once

#include "memory.h"
#include "auto_lock.h"
#include "shared_auto_lock.h"

namespace Core
{

template<typename K, typename V, typename LockType, int T, Memory::PoolType PoolType>
class Btree
{
public:
    Btree()
    {
    }

    virtual ~Btree()
    {
    }

    bool Insert(const K& key, const V& value)
    {
        AutoLock lock(Lock);

        bool exist;
        auto found = LookupLocked(key, exist);
        if (exist)
        {
            return false;
        }

        if (Root.Get() == nullptr)
        {
            Root = MakeShared<BtreeNode, Memory::PoolType::Kernel>(true);
            if (Root.Get() == nullptr)
            {
                return false;
            }
        }

        if (Root->IsFull())
        {
            auto newNode = MakeShared<BtreeNode, Memory::PoolType::Kernel>();
            if (newNode.Get() == nullptr)
            {
                return false;
            }
            auto newNode2 = MakeShared<BtreeNode, Memory::PoolType::Kernel>();
            if (newNode2.Get() == nullptr)
            {
                return false;
            }
            newNode->Child[0] = Root;
            newNode->SplitChild(0, newNode2);
            Root = newNode;
        }

        return Root->InsertNonFull(key, value);
    }

    bool Delete(const K& key)
    {
        AutoLock lock(Lock);
        if (Root.Get() == nullptr)
            return false;

        return false;
    }

    V Lookup(const K& key, bool& exist)
    {
        SharedAutoLock lock(Lock);
        return LookupLocked(key, exist);
    }

private:
    Btree(const Btree& other) = delete;
    Btree(Btree&& other) = delete;
    Btree& operator=(const Btree& other) = delete;
    Btree& operator=(Btree&& other) = delete;

    V LookupLocked(const K& key, bool& exist)
    {
        exist = false;
        if (Root.Get() == nullptr)
        {
            return EmptyValue;
        }

        return Root->Lookup(key, exist);
    }

    class BtreeNode;
    using BtreeNodePtr = SharedPtr<BtreeNode, PoolType>;

    class BtreeNode {
    public:
        BtreeNode(bool leaf = false)
            : KeyCount(0)
            , Leaf(leaf)
        {
        }

        virtual ~BtreeNode()
        {
        }

        bool IsFull()
        {
            return ((2 * T - 1) == KeyCount) ? true : false;
        }

        bool InsertNonFull(const K& key, const V& value)
        {
            return false;
        }

        void MoveKeyValue(int index, const BtreeNodePtr& src, int srcIndex)
        {
            Key[index] = Memory::Move(src->Key[srcIndex]);
            Value[index] = Memory::Move(src->Value[srcIndex]);
        }

        void MoveChild(int index, const BtreeNodePtr& src, int srcIndex)
        {
            Child[index] = Memory::Move(src->Child[srcIndex]);
        }

        void PutKeyValue(int index, const BtreeNodePtr& src, int srcIndex)
        {
            /* free space */
            for (int i = KeyCount; i >= index; i--)
            {
                Key[i + 1] = Memory::Move(Key[i]);
                Value[i + 1] = Memory::Move(Value[i]);
            }

            Key[index] = Memory::Move(src->Key[srcIndex]);
            Value[index] = Memory::Move(src->Value[srcIndex]);
        }

        void PutChild(int index, const BtreeNodePtr& child)
        {
            /* free space */
            for (int i = KeyCount; i >= index; i--)
            {
                Child[i + 1] = Memory::Move(Child[i]);
            }

            Child[index] = child;
        }

        void SplitChild(int index, const BtreeNodePtr& newChild)
        {
            auto child = Child[index];

            newChild->Leaf = child->Leaf;
            /* copy T-1 keys from child to newChild */
            for (int i = 0; i < (T - 1); i++)
            {
                newChild->MoveKeyValue(i,  child, i + T);
            }
            newChild->KeyCount = T - 1;
            /* copy T childs from child to new */
            if (!child->Leaf)
            {
                for (int i = 0; i < T; i++)
                {
                    newChild->MoveChild(i, child, i + T);
                }
            }
            /* shift node childs to the right by one */
            child->KeyCount = T - 1;
            /* setup node new child */
            PutChild(index + 1, newChild);
            /* move mid key from child to node */
            PutKeyValue(index, child, T - 1);
            KeyCount++;
        }

        V Lookup(const K& key, bool& exist)
        {
            BtreeNode *node = this;

            exist = false;
            for (;;)
            {
                if (node->KeyCount == 0)
                    return EmptyValue;

                int i = node->FindKeyIndex(key);
                if (i < node->KeyCount && key == node->Key[i])
                {
                    exist = true;
                    return node->Value[i];
                }
                else if (node->Leaf)
                {
                    return EmptyValue;
                }
                else
                {
                    node = node->Child[i].Get();
                }
            }
        }

        int FindKeyIndex(const K& key)
        {
            int i = 0;
            while (i < KeyCount && key > Key[i])
                i++;

            return i;
        }

        K Key[2 * T - 1];
        V Value[2 * T - 1];
        BtreeNodePtr Child[2 * T];
        int KeyCount;
        bool Leaf;
        V EmptyValue;
    private:
        BtreeNode(const BtreeNode& other) = delete;
        BtreeNode(BtreeNode&& other) = delete;
        BtreeNode& operator=(const BtreeNode& other) = delete;
        BtreeNode& operator=(BtreeNode&& other) = delete;
    };

    BtreeNodePtr Root;
    LockType Lock;
    V EmptyValue;
};

}