#pragma once

#include "vfs_file.h"
#include "error.h"

namespace Core
{

class RandomFile
{
public:
    RandomFile(Error& err, bool pseudoRandom = false);

    Error GetBytes(void* buf, unsigned long len);
    unsigned long GetUlong();

    virtual ~RandomFile();

private:
    RandomFile() = delete;
    RandomFile(const RandomFile& other) = delete;
    RandomFile(RandomFile&& other) = delete;
    RandomFile& operator=(const RandomFile& other) = delete;
    RandomFile& operator=(RandomFile&& other) = delete;

    VfsFile File;
};

class Random
{
public:
    static void GetBytes(void* buf, int len);
private:
    Random() = delete;
    Random(const Random& other) = delete;
    Random(Random&& other) = delete;
    Random& operator=(const Random& other) = delete;
    Random& operator=(Random&& other) = delete;
};

}