#pragma once

#include "kapi.h"
#include "error.h"

namespace Core
{

template<typename T>
static inline Error CopyToUser(T* dst, T* src)
{
    return get_kapi()->copy_to_user(dst, src, sizeof(T));
}

template<typename T>
static inline Error CopyFromUser(T* dst, T* src)
{
    return get_kapi()->copy_from_user(dst, src, sizeof(T));
}

}