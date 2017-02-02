#pragma once

#include "astring.h"
#include "error.h"

namespace Core
{

class VfsFile
{
public:
    VfsFile();
    VfsFile(const AString& path, Error& err, bool read = 0, bool create = 0);
    virtual ~VfsFile();

    Error Open(const AString& path, bool read = 0, bool create = 0);
    Error Read(unsigned long long offset, void* buf, int len);
    Error Write(unsigned long long offset, void* buf, int len);
    Error Sync();
    void Close();

private:
    void* FilePtr;

};

}