#pragma once

#include "memory.h"

void* operator new(size_t size, Core::Memory::PoolType poolType);
void* operator new[](size_t size, Core::Memory::PoolType poolType);
void operator delete(void* ptr);
void operator delete[](void* ptr);
