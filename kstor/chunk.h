#pragma once

#include <core/memory.h>

#include "guid.h"
#include "api.h"

namespace KStor 
{

class Chunk
{
public:
    using Ptr =  Core::SharedPtr<Chunk>;

    Chunk(const Guid& chunkId)
        : ChunkId(chunkId)
    {
    }

    Chunk(const Guid& chunkId, unsigned char data[Api::ChunkSize])
        : Chunk(chunkId)
    {
        Core::Memory::MemCpy(Data, data, sizeof(Data));
    }

    virtual ~Chunk(){}

    Guid ChunkId;
    unsigned char Data[Api::ChunkSize];
private:
    Chunk(const Chunk& other) = delete;
    Chunk(Chunk&& other) = delete;
    Chunk& operator=(const Chunk& other) = delete;
    Chunk& operator=(Chunk&& other) = delete;
};

}
