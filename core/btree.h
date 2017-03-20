#pragma once

#include "memory.h"
#include "auto_lock.h"
#include "shared_auto_lock.h"
#include "unique_ptr.h"
#include "bug.h"
#include "rwsem.h"

namespace Core
{

const int BtreeLL = 254;

template<typename K, typename V, int T,
         typename LockType = RWSem, Memory::PoolType PoolType = Memory::PoolType::Kernel>
class Btree
{
public:
    Btree()
    {
        trace(BtreeLL, "tree 0x%p ctor", this);
    }

    virtual ~Btree()
    {
        trace(BtreeLL, "tree 0x%p dtor", this);
        trace(BtreeLL, "tree 0x%p dtor, root 0x%p", this, Root.Get());
        Root.Reset();
        trace(BtreeLL, "tree 0x%p dtor complete", this);
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
            newNode->PutChild(0, Root);
            newNode->SplitChild(0, newNode2);
            Root = newNode;
        }

        return Root->InsertNonFull(Root, key, value);
    }

    bool Delete(const K& key)
    {
        AutoLock lock(Lock);

        trace(BtreeLL, "root 0x%p", Root.Get());

        BtreeNodePtr node = Root;
        if (node.Get() == nullptr)
        {
            return false;
        }

        KeyToDelete = key;

restart:
        int i = node->GetKeyIndex(KeyToDelete);
        trace(BtreeLL, "node 0x%p get key index %d", node.Get(), i);
        if (i >= 0)
        {
            if (node->IsLeaf())
            {
                trace(BtreeLL, "node 0x%p is leaf", node.Get());
                node->DeleteKey(i);
                node->DecKeyCount();
                return true;
            }
            else
            {
                panic(i < 0);
                BtreeNodePtr preChild = node->GetChild(i);
                BtreeNodePtr sucChild = node->GetChild(i + 1);
                panic(preChild.Get() == nullptr);
                panic(sucChild.Get() == nullptr);

                if (preChild->GetKeyCount() >= T)
                {
                    int preIndex;
                    auto preNode = preChild->FindLeftMost(preChild, preIndex);
                    panic(preNode.Get() == nullptr);
                    KeyToDelete = preNode->GetKey(preIndex);
                    node->CopyKeyValue(i, preNode, preIndex);
                    node = preNode;
                    goto restart;
                }
                else if (sucChild->GetKeyCount() >= T)
                {
                    int sucIndex;
                    auto sucNode = sucChild->FindRightMost(sucChild, sucIndex);
                    panic(sucNode.Get() == nullptr);
                    KeyToDelete = sucNode->GetKey(sucIndex);
                    node->CopyKeyValue(i, sucNode, sucIndex);
                    node = sucNode;
                    goto restart;
                }
                else
                {
                    /* merge key and all of sucChild into preChild
                    * node loses key and pointer to sucChild
                    */
                    int keyIndex = preChild->GetKeyCount();

                    trace(BtreeLL, "node 0x%p pre child 0x%p merge keyIndex %d",
                        node.Get(), preChild.Get(), keyIndex);

                    preChild->Merge(sucChild,
                                    Memory::Move(node->GetKey(i)),
                                    Memory::Move(node->GetValue(i)));
                    node->DeleteKey(i);
                    node->DeleteChild(i + 1);
                    node->DecKeyCount();
                    /* delete sucChild */
                    sucChild.Reset();
                    if (node->GetKeyCount() == 0)
                    {
                        node->Copy(preChild);
                        /* delete preChild */
                        preChild.Reset();
                    } else {
                        node = preChild;
                    }

                    trace(BtreeLL, "node 0x%p get key %d", node.Get(), keyIndex);

                    KeyToDelete = node->GetKey(keyIndex);
                    goto restart;
                }
            }
        }
        else
        {
            if (node->IsLeaf())
            {
                trace(BtreeLL, "node 0x%p is leaf", node.Get());
                return false;
            }
            else
            {
                i = node->FindKeyIndex(KeyToDelete);
                trace(BtreeLL, "node 0x%p find key index %d", node.Get(), i);
                node = node->ChildBalance(node, i);
                panic(node.Get() == nullptr);
                goto restart;
            }
        }
        return false;
    }

    V Lookup(const K& key, bool& exist)
    {
        SharedAutoLock lock(Lock);
        return LookupLocked(key, exist);
    }

    bool Check()
    {
        SharedAutoLock lock(Lock);
        if (Root.Get() == nullptr)
            return true;

        return Root->Check(true);
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

        BtreeNodePtr node = Root;
        for (;;)
        {
            if (node->GetKeyCount() == 0)
                return EmptyValue;

            int i = node->FindKeyIndex(key);
            if (i < node->GetKeyCount() && key == node->GetKey(i))
            {
                exist = true;
                return node->GetValue(i);
            }
            else if (node->IsLeaf())
            {
                return EmptyValue;
            }
            else
            {
                node = node->GetChild(i);
                panic(node.Get() == nullptr);
            }
        }
    }

    class BtreeNode;
    using BtreeNodePtr = SharedPtr<BtreeNode>;

    class BtreeNode {
    public:
        BtreeNode(bool leaf = false)
        {
            trace(BtreeLL, "node 0x%p ctor", this);
            SetLeaf(leaf);
            SetKeyCount(0);
        }

        virtual ~BtreeNode()
        {
            trace(BtreeLL, "node 0x%p dtor", this);
            for (int i = 0; i < (2 * T); i++)
            {
                if (Child[i].Get() != nullptr)
                {
                    trace(BtreeLL, "node 0x%p dtor, reset child 0x%p", this, Child[i].Get());
                    Child[i].Reset();
                }
            }
            trace(BtreeLL, "node 0x%p dtor complete", this);
        }

        bool IsFull()
        {
            bool result = ((2 * T - 1) == KeyCount) ? true : false;
            trace(BtreeLL, "node 0x%p full %d keyCount %d", this, result, KeyCount);
            return result;
        }

        void CopyKeyValue(int index, const BtreeNodePtr& src, int srcIndex)
        {
            panic(index < 0 || index >= (2 * T - 1));
            panic(srcIndex < 0 || srcIndex >= (2 * T - 1));

            Key[index] = Memory::Move(src->Key[srcIndex]);
            Value[index] = Memory::Move(src->Value[srcIndex]);
        }

        void CopyChild(int index, const BtreeNodePtr& src, int srcIndex)
        {
            panic(index < 0 || index >= 2 * T);
            panic(srcIndex < 0 || srcIndex >= 2 * T);

            Child[index] = Memory::Move(src->Child[srcIndex]);
        }

        void PutKeyValue(int index, const BtreeNodePtr& src, int srcIndex)
        {
            panic(index < 0 || index >= (2 * T - 1));
            panic(srcIndex < 0 || srcIndex >= (2 * T - 1));

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
            panic(index < 0 || index >= (2 * T - 1));

            /* free space */
            for (int i = KeyCount - 1; i >= index; i--)
            {
                Key[i + 1] = Memory::Move(Key[i]);
                Value[i + 1] = Memory::Move(Value[i]);
            }

            Key[index] = key;
            Value[index] = value;
        }

        void PutChild(int index, const BtreeNodePtr& child)
        {
            panic(index < 0 || index >= 2 * T);

            /* free space */
            for (int i = KeyCount; i >= index; i--)
            {
                Child[i + 1] = Memory::Move(Child[i]);
            }

            Child[index] = child;
        }

        void PutChild(int index, const BtreeNodePtr& src, int srcIndex)
        {
            panic(index < 0 || index >= 2 * T);
            panic(srcIndex < 0 || srcIndex >= 2 * T);

            PutChild(index, src->Child[srcIndex]);
        }

        bool IsLeaf()
        {
            return Leaf;
        }

        void SetLeaf(bool leaf)
        {
            Leaf = leaf;
            trace(BtreeLL, "node 0x%p set leaf %d", this, leaf);
        }

        void SetKeyCount(int keyCount)
        {
            panic(keyCount < 0 || keyCount > (2 * T - 1));

            KeyCount = keyCount;
            trace(BtreeLL, "node 0x%p set keyCount %d", this, keyCount);
        }

        void IncKeyCount(int delta = 1)
        {
            SetKeyCount(KeyCount + delta);
        }

        void DecKeyCount()
        {
            SetKeyCount(KeyCount - 1);
        }

        void DeleteChild(int index)
        {
            panic(index < 0 || index >= 2 * T);

            for (int i = (index + 1); i < (KeyCount + 1); i++)
            {
                Child[i - 1] = Memory::Move(Child[i]);
            }
        }

        void DeleteKey(int index)
        {
            panic(index < 0 || index >= (2 * T - 1));

            for (int i = (index + 1); i < KeyCount; i++)
            {
                Key[i - 1] = Memory::Move(Key[i]);
                Value[i - 1] = Memory::Move(Value[i]);
            }
        }

        void SplitChild(int index, const BtreeNodePtr& newChild)
        {
            trace(BtreeLL, "node 0x%p SplitChild %d newChild 0x%p",
                this, index, newChild.Get());

            auto child = Child[index];

            panic(child.Get() == nullptr);
            panic(newChild.Get() == nullptr);

            newChild->SetLeaf(child->Leaf);
            /* copy T-1 keys from child to newChild */
            for (int i = 0; i < (T - 1); i++)
            {
                newChild->CopyKeyValue(i,  child, i + T);
            }
            newChild->SetKeyCount(T - 1);
            /* copy T childs from child to new */
            if (!child->Leaf)
            {
                for (int i = 0; i < T; i++)
                {
                    newChild->CopyChild(i, child, i + T);
                }
            }
            /* shift node childs to the right by one */
            child->SetKeyCount(T - 1);
            /* setup node new child */
            PutChild(index + 1, newChild);
            /* move mid key from child to node */
            PutKeyValue(index, child, T - 1);
            IncKeyCount();
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

        bool InsertNonFull(const BtreeNodePtr& self, const K& key, const V& value)
        {
            panic(self.Get() != this);

            BtreeNodePtr node = self;

            for (;;)
            {
                auto i = node->GetKeyIndex(key);
                if (i >= 0)
                {
                    return false;
                }

                i = node->FindKeyIndex(key);
                if (node->IsLeaf())
                {
                    node->PutKeyValue(i, key, value);
                    node->IncKeyCount();
                    return true;
                }
                else
                {
                    auto child = node->GetChild(i);
                    panic(child.Get() == nullptr);
                    if (child->IsFull())
                    {
                        auto newNode = MakeShared<BtreeNode, Memory::PoolType::Kernel>();
                        if (newNode.Get() == nullptr)
                        {
                            return false;
                        }
                        node->SplitChild(i, newNode);
                        continue; /* restart */
                    }
                    node = child;
                }
            }
        }

        BtreeNodePtr FindLeftMost(const BtreeNodePtr& self, int& index)
        {
            panic(self.Get() != this);

            trace(BtreeLL, "node 0x%p find left most", this);

            BtreeNodePtr curr = self;

            for (;;)
            {
                panic(curr->KeyCount == 0);
                if (curr->Leaf)
                {
                    index = curr->KeyCount - 1;
                    return curr;
                }
                curr = curr->ChildBalance(curr, curr->KeyCount);
            }
        }

        BtreeNodePtr FindRightMost(const BtreeNodePtr& self, int& index)
        {
            panic(self.Get() != this);

            trace(BtreeLL, "node 0x%p find right most", this);

            BtreeNodePtr curr = self;

            for (;;)
            {
                panic(curr->KeyCount == 0);
                if (curr->Leaf)
                {
                    index = curr->KeyCount - 1;
                    return curr;
                }
                curr = curr->ChildBalance(curr, 0);
            }
        }

        void Merge(const BtreeNodePtr& src, K&& key, V&& value)
        {
            Key[KeyCount] = Memory::Move(key);
            Value[KeyCount] = Memory::Move(value);

            int pos = KeyCount + 1, i;
            for (i = 0; i < src->KeyCount; i++, pos++)
            {
                Key[pos] = Memory::Move(src->Key[i]);
                Value[pos] = Memory::Move(src->Value[i]);
                Child[pos] = Memory::Move(src->Child[i]);
            }

            Child[pos] = Memory::Move(src->Child[i]);
            IncKeyCount(1 + src->KeyCount);
        }

        void Copy(const BtreeNodePtr& src)
        {
            int i;
            for (i = 0; i < src->GetKeyCount(); i++)
            {
                Key[i] = Memory::Move(src->GetKey(i));
                Value[i] = Memory::Move(src->GetValue(i));
            }
            Child[i] = Memory::Move(src->GetChild(i));
            Leaf = src->IsLeaf();
            SetKeyCount(src->GetKeyCount());
        }

        void ChildGiveKey(const BtreeNodePtr& self, int childIndex, int sibIndex, bool left)
        {
            /* give child an extra key by moving a key from node
             * down into child, moving a key from child's
             * immediate left or right sibling up into node,
             * and moving the appropriate child pointer from the
             * sibling into child
             */

            panic(self.Get() != this);
            panic(childIndex < 0 || childIndex >= 2*T);
            panic(sibIndex < 0 || sibIndex >= 2 *T);

            auto child = Child[childIndex];
            auto sib = Child[sibIndex];
            panic(child.Get() == nullptr);
            panic(sib.Get() == nullptr);
            panic(child.Get() == sib.Get());

            if (!left)
            {
                child->Key[child->KeyCount] = Memory::Move(Key[childIndex]);
                child->Value[child->KeyCount] = Memory::Move(Value[childIndex]);

                Key[childIndex] = Memory::Move(sib->GetKey(0));
                Value[childIndex] = Memory::Move(sib->GetValue(0));

                child->Child[child->KeyCount + 1] = Memory::Move(sib->GetChild(0));

                sib->DeleteKey(0);
                sib->DeleteChild(0);
            }
            else
            {
                panic(childIndex == 0);
                panic(sib->KeyCount == 0);

                child->PutKeyValue(0, self, childIndex - 1);
                Key[childIndex - 1] = Memory::Move(sib->GetKey(sib->KeyCount - 1));
                Value[childIndex - 1] = Memory::Move(sib->GetValue(sib->KeyCount - 1));
                child->PutChild(0, sib, sib->KeyCount);

                sib->DeleteKey(sib->KeyCount - 1);
                sib->DeleteChild(sib->KeyCount);
            }
            child->IncKeyCount();
            sib->DecKeyCount();
        }

        BtreeNodePtr ChildMerge(const BtreeNodePtr& self, int childIndex, int sibIndex, bool left)
        {
            panic(self.Get() != this);
            panic(childIndex < 0 || childIndex >= 2*T);
            panic(sibIndex < 0 || sibIndex >= 2 *T);

            auto child = Child[childIndex];
            auto sib = Child[sibIndex];

            panic(child.Get() == nullptr);
            panic(sib.Get() == nullptr);
            panic(child.Get() == sib.Get());

            if (left)
            {
                panic(childIndex == 0);
                sib->Merge(child,
                        Memory::Move(Key[childIndex-1]),
                        Memory::Move(Value[childIndex - 1]));
                DeleteKey(childIndex - 1);
                DeleteChild(childIndex);
                DecKeyCount();
                child.Reset();
            }
            else
            {
                child->Merge(sib,
                        Memory::Move(Key[childIndex]),
                        Memory::Move(Value[childIndex]));
                DeleteKey(childIndex);
                DeleteChild(childIndex + 1);
                DecKeyCount();
                sib.Reset();
            }

            if (KeyCount == 0)
            {
                if (left)
                {
                    Copy(sib);
                    sib.Reset();
                }
                else
                {
                    Copy(child);
                    child.Reset();
                }

                return self;
            }
            else
            {
                return (left) ? sib : child;
            }
        }

        BtreeNodePtr ChildBalance(const BtreeNodePtr& self, int childIndex)
        {
            trace(BtreeLL, "node 0x%p child %d balance", this, childIndex);

            panic(self.Get() != this);
            panic(Leaf);
            panic(childIndex < 0 || childIndex >= 2*T);

            auto child = Child[childIndex];
            panic(child.Get() == nullptr);

            if (child->KeyCount < T)
            {
                auto left = (childIndex > 0) ?
                    GetChild(childIndex - 1) : BtreeNodePtr();
                auto right = (childIndex < KeyCount) ?
                    GetChild(childIndex + 1) : BtreeNodePtr();

                if (left.Get() != nullptr && left->KeyCount >= T)
                {
                    ChildGiveKey(self, childIndex, childIndex - 1, true);
                }
                else if (right.Get() != nullptr && right->KeyCount >= T)
                {
                    ChildGiveKey(self, childIndex, childIndex + 1, false);
                }
                else if (left.Get() != nullptr && left->KeyCount < T)
                {
                    return ChildMerge(self, childIndex, childIndex - 1, true);
                }
                else if (right.Get() != nullptr && right->KeyCount < T)
                {
                    return ChildMerge(self, childIndex, childIndex + 1, false);
                }
                else
                {
                    panic(true);
                    return BtreeNodePtr();
                }
            }

            return child;
        }

        BtreeNodePtr GetChild(int childIndex)
        {
            trace(BtreeLL, "node 0x%p get child %d", this, childIndex);

            panic(childIndex < 0 || childIndex >= 2*T);
            return Child[childIndex];
        }

        K& GetKey(int keyIndex)
        {
            trace(BtreeLL, "node 0x%p get key %d", this, keyIndex);

            panic(keyIndex < 0 || keyIndex >= (2*T - 1));
            panic(keyIndex >= KeyCount);
            return Key[keyIndex];
        }

        V& GetValue(int keyIndex)
        {
            trace(BtreeLL, "node 0x%p get value %d", this, keyIndex);

            panic(keyIndex < 0 || keyIndex >= (2*T - 1));
            panic(keyIndex >= KeyCount);
            return Value[keyIndex];
        }

        int GetKeyCount()
        {
            return KeyCount;
        }

        bool Check(bool root)
        {
            if (KeyCount < 0 || KeyCount > (2 * T - 1))
                return false;

            if (!root)
            {
                if (KeyCount < (T - 1))
                    return false;
            }

            K* prevKey = nullptr;
            for (int i = 0; i < KeyCount; i++)
            {
                if (prevKey != nullptr && *prevKey >= Key[i])
                    return false;
                prevKey = &Key[i];
            }

            if (!Leaf)
            {
                for (int i = 0; i < KeyCount + 1; i++)
                {
                    if (Child[i].Get() == nullptr)
                        return false;

                    auto result = Child[i]->Check(false);
                    if (!result)
                        return false;
                }

                for (int i = KeyCount + 1; i < 2 * T; i++)
                {
                    if (Child[i].Get() != nullptr)
                        return false;
                }
            }
            else
            {
                for (int i = 0; i < 2 * T; i++)
                {
                    if (Child[i].Get() != nullptr)
                        return false;
                }
            }

            return true;
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
    K KeyToDelete;
};

}