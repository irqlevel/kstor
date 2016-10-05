#pragma once

#include "memory.h"
#include "error.h"
#include "random.h"

class Page
{
public:
    Page(Memory::PoolType poolType, Error& err);
    void* Map();
    void Unmap();
    void* MapAtomic();
    void UnmapAtomic(void* va);

    void* GetPage();
    int GetPageSize();

    void Zero();
    Error FillRandom(Random& rng);
    int CompareContent(Page& other);

    virtual ~Page();

private:
    Page(const Page& other) = delete;
    Page(Page&& other) = delete;
    Page& operator=(const Page& other) = delete;
    Page& operator=(Page&& other) = delete;

    Memory::PoolType PoolType;
    void* PagePtr;
};