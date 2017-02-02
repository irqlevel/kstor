#pragma once

namespace KStor
{

namespace Control
{

template <typename T,unsigned S>
inline unsigned ArraySize(const T (&v)[S])
{
    return S;
}

}

}