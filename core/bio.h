#pragma once

#include "error.h"
#include "memory.h"
#include "page.h"
#include "kapi.h"
#include "block_device.h"

class Bio
{
public:
    Bio(int pageCount, Error& err);
    void SetBdev(BlockDevice& blockDevice);
    virtual ~Bio();
private:
    void* BioPtr;
    int PageCount;
};