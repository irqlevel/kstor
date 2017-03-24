#include "vfs_file.h"
#include "kapi.h"

namespace Core
{

VfsFile::VfsFile()
    : FilePtr(nullptr)
{
}

Error VfsFile::Open(const AString& path, bool read, bool create)
{
    if (FilePtr != nullptr)
    {
        return Error::InvalidState;
    }

    int flags = 0;
    if (read)
    {
        flags |= KAPI_VFS_FILE_RDONLY;
    }
    else
    {
        flags |= KAPI_VFS_FILE_RDWR;
    }
    if (create)
    {
        flags |= (KAPI_VFS_FILE_CREAT | KAPI_VFS_FILE_EXCL);
    }

    int rc = get_kapi()->vfs_file_open(path.GetConstBuf(), flags, &FilePtr);
    if (rc)
    {
        trace(0, "Can't open file %s, rc %d", path.GetConstBuf(), rc);
        return MakeError(rc);
    }

    return MakeError(Error::Success);
}

VfsFile::VfsFile(const AString& path, Error& err, bool read, bool create)
    : VfsFile()
{
    if (!err.Ok())
    {
        return;
    }

    err = Open(path, read, create);
    return;
}

VfsFile::~VfsFile()
{
    Close();
}

Error VfsFile::Read(unsigned long long offset, void* buf, unsigned long len)
{
    return get_kapi()->vfs_file_read(FilePtr, buf, len, offset);
}

Error VfsFile::Write(unsigned long long offset, const void* buf, unsigned long len)
{
    return get_kapi()->vfs_file_write(FilePtr, buf, len, offset);
}

Error VfsFile::Sync()
{
    return get_kapi()->vfs_file_sync(FilePtr);
}

void VfsFile::Close()
{
    if (FilePtr != nullptr)
    {
        get_kapi()->vfs_file_close(FilePtr);
        FilePtr = nullptr;
    }
}

}