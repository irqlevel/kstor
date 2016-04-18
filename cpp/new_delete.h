#pragma once

#include "mem_type.h"

void* operator new(size_t size, MemType memType);
void* operator new[](size_t size, MemType memType);
void operator delete(void* ptr);
void operator delete[](void* ptr);
