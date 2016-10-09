#pragma once

#include "kapi.h"
#include "pool_type.h"

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

    template <class T> void Swap(T& a, T& b)
    {
        T c(Move(a)); a=Move(b); b=Move(c);
    }

    static inline void MemSet(void* ptr, int c, size_t size)
    {
        get_kapi()->memset(ptr, c, size);
    }

    static inline int MemCmp(void* ptr1, void* ptr2, size_t size)
    {
        return get_kapi()->memcmp(ptr1, ptr2, size);
    }

    static inline void MemCpy(void* dst, void* src, size_t size)
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

    template <typename T,unsigned S>
    inline unsigned ArraySize(const T (&v)[S])
    {
        return S;
    }
}
