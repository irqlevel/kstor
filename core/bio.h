#pragma once

#include "error.h"
#include "memory.h"
#include "page.h"

class Bio
{
public:
    Bio(Error& err);
    virtual ~Bio();
private:
    void* BioPtr;
};