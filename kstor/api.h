#pragma once

namespace KStor 
{

namespace Api 
{

const unsigned int PacketMagic = 0xCCBECCBE;
const unsigned int PacketMaxDataSize = 2 * 65536;

const unsigned int GuidSize = 16;
const unsigned int VolumeMagic = 0xCBDACBDA;

const unsigned int PacketTypePing = 1;
const unsigned int PacketTypeChunkCreate = 2;
const unsigned int PacketTypeChunkWrite = 3;
const unsigned int PacketTypeChunkRead = 4;
const unsigned int PacketTypeChunkDelete = 5;

const unsigned int ChunkSize = 65536;

const unsigned int ResultSuccess = 0;
const unsigned int ResultUnexpectedDataSize = 1;
const unsigned int ResultNotFound = 2;

#pragma pack(push, 1)

struct PacketHeader
{
    unsigned int Magic;
    unsigned int Type;
    unsigned int DataSize;
    unsigned int Result;
};

struct Guid
{
    unsigned char Data[GuidSize];
};

struct VolumeHeader
{
    unsigned int Magic;
    unsigned char Padding[12];
    Guid VolumeId;
};

struct ChunkCreateRequest
{
    Guid ChunkId;
};

struct ChunkWriteRequest
{
    Guid ChunkId;
    unsigned char Data[ChunkSize];
};

struct ChunkReadRequest
{
    Guid ChunkId;
};

struct ChunkReadResponse
{
    unsigned char Data[ChunkSize];
};

struct ChunkDeleteRequest
{
    Guid ChunkId;
};

#pragma pack(pop)

}

}