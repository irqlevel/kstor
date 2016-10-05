#pragma once

#include "error.h"
#include "memory.h"
#include "page.h"
#include "kapi.h"
#include "block_device.h"
#include "event.h"

class Bio
{
public:
    Bio(int pageCount, Error& err);
    void SetBdev(BlockDevice& blockDevice);
    void SetRead();
    void SetWrite();
    void SetFua();
    void SetFlush();
    void SetPosition(unsigned long long sector);
    Error SetPage(int pageIndex, Page& page, int offset, int len);
    void Submit();
    void Wait();
    Error GetError();
    virtual ~Bio();

private:
    void EndIo(int err);
    static void EndIo(void* bio, int err);
    void* BioPtr;
    int PageCount;
    Event EndIoEvent;
    Error IoError;
};