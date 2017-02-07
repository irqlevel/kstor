#pragma once

#include "vfs_file.h"
#include "error.h"

namespace Core
{

class Random
{
public:
    Random(Error& err, bool pseudoRandom = false);
    Error GetBytes(void* buf, unsigned long len);
    unsigned long GetUlong();
    virtual ~Random();

private:
    VfsFile DevRandomFile;
};

}