#pragma once

namespace KStor 
{

namespace Api 
{

const unsigned int PacketMagic = 0xCCBECCBE;
const unsigned int PacketMaxDataSize = 2 * 65536;

const unsigned int GuidSize = 16;
const unsigned int VolumeMagic = 0xCBDACBDA;
const unsigned int JournalMagic = 0xBCDEBCDE;

const unsigned int PacketTypePing = 1;
const unsigned int PacketTypeChunkCreate = 2;
const unsigned int PacketTypeChunkWrite = 3;
const unsigned int PacketTypeChunkRead = 4;
const unsigned int PacketTypeChunkDelete = 5;

const unsigned int ChunkSize = 65536;

const unsigned int ResultSuccess = 0;
const unsigned int ResultUnexpectedDataSize = 1;
const unsigned int ResultNotFound = 2;

const unsigned int HashSize = 8;

const unsigned int PageSize = 4096;

#pragma pack(push, 1)

struct PacketHeader
{
    unsigned int Magic;
    unsigned int Type;
    unsigned int DataSize;
    unsigned int Result;
    unsigned char DataHash[HashSize];
    unsigned char Hash[HashSize];
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
    unsigned long long BlockSize;
    unsigned long long Size;
    unsigned long long JournalSize;
    unsigned long long BitmapSize;
    unsigned char Unused[PageSize - 2 * 16 - 4 * 8 - 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(VolumeHeader) == PageSize, "Bad size");

struct JournalHeader
{
    unsigned int Magic;
    unsigned char Padding[12];
    unsigned long long Size;
    unsigned char Unused[PageSize - 2 * 16];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalHeader) == PageSize, "Bad size");

struct JournalDescBlock
{
    Guid TransactionId;
    unsigned char Unused[PageSize - 16 - 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalDescBlock) == PageSize, "Bad size");

struct JournalDataBlock
{
    Guid TransactionId;
    unsigned char Data[PageSize - 16 - 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalDataBlock) == PageSize, "Bad size");

struct JournalCommitBlock
{
    Guid TransactionId;
    unsigned char Unused[PageSize - 16 - 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalCommitBlock) == PageSize, "Bad size");

struct BitmapBlock
{
    unsigned char Data[PageSize - 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(BitmapBlock) == PageSize, "Bad size");

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