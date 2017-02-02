#pragma once

namespace Core
{

namespace Memory
{
    enum class PoolType
    {
        Atomic,
        Kernel,
        NoFS,
        NoIO,
        User,
        Undefined
    };
}

}