#pragma once

#include "kapi.h"
#include "pool_type.h"
#include "type.h"

namespace Core
{

namespace Memory
{

    template< class T > struct RemoveReference      {typedef T type;};
    template< class T > struct RemoveReference<T&>  {typedef T type;};
    template< class T > struct RemoveReference<T&&> {typedef T type;};

    template <typename T>
    typename RemoveReference<T>::type&& Move(T&& arg)
    {
        return static_cast<typename RemoveReference<T>::type&&>(arg);
    }

    template <class T>
    T&& Forward(typename RemoveReference<T>::type& arg)
    {
        return static_cast<T&&>(arg);
    }

    template <class T>
    T&& Forward(typename RemoveReference<T>::type&& arg)
    {
        return static_cast<T&&>(arg);
    }

    template <class T> void Swap(T& a, T& b)
    {
        T c(Move(a)); a=Move(b); b=Move(c);
    }

    static inline void *MemAdd(void *ptr, unsigned long len)
    {
        return reinterpret_cast<void *>(reinterpret_cast<unsigned long>(ptr) + len);
    }

    static inline const void *MemAdd(const void *ptr, unsigned long len)
    {
        return reinterpret_cast<const void *>(reinterpret_cast<unsigned long>(ptr) + len);
    }

    static inline void MemSet(void* ptr, int c, size_t size)
    {
        get_kapi()->memset(ptr, c, size);
    }

    static inline int MemCmp(const void* ptr1, const void* ptr2, size_t size)
    {
        return get_kapi()->memcmp(ptr1, ptr2, size);
    }

    static inline void MemCpy(void* dst, const void* src, size_t size)
    {
        get_kapi()->memcpy(dst, src, size);
    }

    static inline size_t StrLen(const char* s)
    {
        size_t i = 0;
        while (s[i] != 0)
        {
            i++;
        }
        return i;
    }

    template <typename T,unsigned S> unsigned ArraySize(const T (&v)[S])
    {
        return S;
    }

    template <typename T,unsigned S> bool ArrayEqual(const T (&s1)[S], const T (&s2)[S])
    {
        for (size_t i = 0; i < S; i++)
        {
            if (s1[i] != s2[i])
                return false;
        }

        return true;
    }

    static const unsigned int IntBitCount = 8 * sizeof(int);
    static const unsigned int LongBitCount = 8 * sizeof(unsigned long);
    static const unsigned int MaxInt = (static_cast<unsigned int>(1) << (IntBitCount - 1)) - 1;

    static inline void *Malloc(size_t size, Core::Memory::PoolType poolType)
    {
        return get_kapi()->kmalloc(size, get_kapi_pool_type(poolType));
    }

    static inline void Free(void *ptr)
    {
        get_kapi()->kfree(ptr);
    }

}

}