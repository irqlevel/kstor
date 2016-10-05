#pragma once

#include "vfs_file.h"
#include "error.h"

class Random
{
public:
    Random(Error& err, bool pseudoRandom = false);
    Error GetBytes(void* buf, int len);
    unsigned long GetUlong();
    virtual ~Random();

private:
    VfsFile DevRandomFile;
};