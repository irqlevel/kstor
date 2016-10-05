#pragma once

#include "kapi.h"

namespace util
{

    template< class T > struct remove_reference      {typedef T type;};
    template< class T > struct remove_reference<T&>  {typedef T type;};
    template< class T > struct remove_reference<T&&> {typedef T type;};

    template <typename T>
    typename remove_reference<T>::type&& move(T&& arg)
    {
        return static_cast<typename remove_reference<T>::type&&>(arg);
    }

    template <class T> void swap (T& a, T& b)
    {
        T c(move(a)); a=move(b); b=move(c);
    }

    static inline void memset(void* ptr, int c, size_t size)
    {
        get_kapi()->memset(ptr, c, size);
    }

    static inline int memcmp(void* ptr1, void* ptr2, size_t size)
    {
        return get_kapi()->memcmp(ptr1, ptr2, size);
    }

    static inline void memcpy(void* dst, void* src, size_t size)
    {
        get_kapi()->memcpy(dst, src, size);
    }

}
