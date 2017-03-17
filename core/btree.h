#pragma once

#include "memory.h"
#include "auto_lock.h"
#include "shared_auto_lock.h"
#include "unique_ptr.h"
#include "bug.h"

namespace Core
{

const int BtreeLL = 7;

template<typename K, typename V, typename LockType, int T, Memory::PoolType PoolType>
class Btree
{
public:
    Btree()
    {
        trace(BtreeLL, "Btree 0x%p ctor", this);
    }

    virtual ~Btree()
    {
        trace(BtreeLL, "Btree 0x%p dtor", this);
        trace(BtreeLL, "Btree 0x%p dtor, root 0x%p", this, Root.Get());
        Root.Reset();
        trace(BtreeLL, "Btree 0x%p dtor complete", this);
    }

    bool Insert(const K& key, const V& value)
    {
        AutoLock lock(Lock);

        bool exist;
        LookupLocked(key, exist);
        if (exist)
        {
            return false;
        }

        if (Root.Get() == nullptr)
        {
            Root = MakeUnique<BtreeNode, Memory::PoolType::Kernel>(true);
            if (Root.Get() == nullptr)
            {
                return false;
            }
        }

        if (Root->IsFull())
        {
            auto newNode = MakeUnique<BtreeNode, Memory::PoolType::Kernel>();
            if (newNode.Get() == nullptr)
            {
                return false;
            }
            auto newNode2 = MakeUnique<BtreeNode, Memory::PoolType::Kernel>();
            if (newNode2.Get() == nullptr)
            {
                return false;
            }
            newNode->PutChild(0, Memory::Move(Root));
            newNode->SplitChild(0, Memory::Move(newNode2));
            Root = Memory::Move(newNode);
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
    using BtreeNodePtr = UniquePtr<BtreeNode>;

    class BtreeNode {
    public:
        BtreeNode(bool leaf = false)
        {
            trace(BtreeLL, "BtreeNode 0x%p ctor", this);
            SetLeaf(leaf);
            SetKeyCount(0);
        }

        virtual ~BtreeNode()
        {
            trace(BtreeLL, "BtreeNode 0x%p dtor", this);
            for (int i = 0; i < (2 * T); i++)
            {
                trace(BtreeLL, "BtreeNode 0x%p dtor, reset child 0x%p", this, Child[i].Get());
                Child[i].Reset();
            }
            trace(BtreeLL, "BtreeNode 0x%p dtor complete", this);
        }

        bool IsFull()
        {
            bool result = ((2 * T - 1) == KeyCount) ? true : false;
            trace(BtreeLL, "BtreeNode 0x%p full %d keyCount %d", this, result, KeyCount);
            return result;
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
            for (int i = KeyCount - 1; i >= index; i--)
            {
                Key[i + 1] = Memory::Move(Key[i]);
                Value[i + 1] = Memory::Move(Value[i]);
            }

            Key[index] = Memory::Move(src->Key[srcIndex]);
            Value[index] = Memory::Move(src->Value[srcIndex]);
        }

        void PutKeyValue(int index, const K& key, const V& value)
        {
            /* free space */
            for (int i = KeyCount - 1; i >= index; i--)
            {
                Key[i + 1] = Memory::Move(Key[i]);
                Value[i + 1] = Memory::Move(Value[i]);
            }

            Key[index] = key;
            Value[index] = value;
        }

        void PutChild(int index, BtreeNodePtr&& child)
        {
            /* free space */
            for (int i = KeyCount; i >= index; i--)
            {
                Child[i + 1] = Memory::Move(Child[i]);
            }

            Child[index] = Memory::Move(child);
        }

        void SetLeaf(bool leaf)
        {
            Leaf = leaf;
            trace(BtreeLL, "BtreeNode 0x%p set leaf %d", this, leaf);
        }

        void SetKeyCount(int keyCount)
        {
            KeyCount = keyCount;
            trace(BtreeLL, "BtreeNode 0x%p set keyCount %d", this, keyCount);
        }

        void IncKeyCount()
        {
            KeyCount++;
            trace(BtreeLL, "BtreeNode 0x%p inc keyCount %d", this, KeyCount);
        }

        void SplitChild(int index, BtreeNodePtr&& newChild)
        {
            trace(BtreeLL, "BtreeNode 0x%p SplitChild %d newChild 0x%p",
                this, index, newChild.Get());

            auto& child = Child[index];

            panic(child.Get() == nullptr);
            panic(newChild.Get() == nullptr);

            newChild->SetLeaf(child->Leaf);
            /* copy T-1 keys from child to newChild */
            for (int i = 0; i < (T - 1); i++)
            {
                newChild->MoveKeyValue(i,  child, i + T);
            }
            newChild->SetKeyCount(T - 1);
            /* copy T childs from child to new */
            if (!child->Leaf)
            {
                for (int i = 0; i < T; i++)
                {
                    newChild->MoveChild(i, child, i + T);
                }
            }
            /* shift node childs to the right by one */
            child->SetKeyCount(T - 1);
            /* setup node new child */
            PutChild(index + 1, Memory::Move(newChild));
            /* move mid key from child to node */
            PutKeyValue(index, child, T - 1);
            IncKeyCount();
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
                    panic(node == nullptr);
                }
            }
        }

        int FindKeyIndex(const K& key)
        {
            if (KeyCount == 0)
                return 0;

            int start = 0;
            int end = KeyCount;
            if (key > Key[end - 1])
                return end;
            else if (key < Key[start])
                return 0;

            while (start < end)
            {
                int mid = (start + end) / 2;
                if (key == Key[mid])
                    return mid;
                else if (key < Key[mid])
                    end = mid;
                else
                    start = mid + 1;
            }

            return end;
        }

        bool IsKeyProbablyInside(const K& key)
        {
            if (KeyCount == 0)
                return false;
            else if ((key > Key[KeyCount-1]) || (key < Key[0]))
                return false;

            return true;
        }

        int GetKeyIndex(const K& key)
        {
            if (!IsKeyProbablyInside(key))
                return -1;

            int start = 0;
            int end = KeyCount;
            while (start < end)
            {
                int mid = (start + end) / 2;
                if (key == Key[mid])
                    return mid;
                else if (key < Key[mid])
                    end = mid;
                else
                    start = mid + 1;
            }

            return -1;
        }

        bool InsertNonFull(const K& key, const V& value)
        {
            BtreeNode* node = this;

            for (;;)
            {
                auto i = node->GetKeyIndex(key);
                if (i >= 0)
                {
                    return false;
                }

                i = node->FindKeyIndex(key);
                if (node->Leaf)
                {
                    node->PutKeyValue(i, key, value);
                    node->IncKeyCount();
                    return true;
                }
                else
                {
                    auto child = node->Child[i].Get();
                    panic(child == nullptr);
                    if (child->IsFull())
                    {
                        auto newNode = MakeUnique<BtreeNode, Memory::PoolType::Kernel>();
                        if (newNode.Get() == nullptr)
                        {
                            return false;
                        }
                        node->SplitChild(i, Memory::Move(newNode));
                        continue; /* restart */
                    }
                    node = child;
                }
            }
        }

    private:
        BtreeNode(const BtreeNode& other) = delete;
        BtreeNode(BtreeNode&& other) = delete;
        BtreeNode& operator=(const BtreeNode& other) = delete;
        BtreeNode& operator=(BtreeNode&& other) = delete;

        K Key[2 * T - 1];
        V Value[2 * T - 1];
        BtreeNodePtr Child[2 * T];
        V EmptyValue;
        int KeyCount;
        bool Leaf;
    };

    BtreeNodePtr Root;
    LockType Lock;
    V EmptyValue;
};

}