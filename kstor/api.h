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
const unsigned int JournalCommitMagic = 0xCFEDCFED;

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

const unsigned int JournalBlockTypeTxBegin = 1;
const unsigned int JournalBlockTypeTxData = 2;
const unsigned int JournalBlockTypeTxCommit = 3;

const unsigned int JournalTxStateNew = 1;
const unsigned int JournalTxStateCommiting = 2;
const unsigned int JournalTxStateCommited = 3;
const unsigned int JournalTxStateCanceled = 4;
const unsigned int JournalTxStateFinished = 5;

const unsigned int TestJournal = 1;

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
    unsigned long long Size;
    unsigned long long JournalSize;
    unsigned long long BitmapSize;
    unsigned char Unused[PageSize - 2 * 16 - 4 * 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(VolumeHeader) == PageSize, "Bad size");

struct JournalHeader
{
    unsigned int Magic;
    unsigned char Padding[12];
    unsigned long long Size;
    unsigned long long LogStartIndex;
    unsigned long long LogEndIndex;
    unsigned long long LogSize;
    unsigned long long LogCapacity;
    unsigned char Unused[PageSize - 2 * 16 - 4 * 8];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalHeader) == PageSize, "Bad size");

struct JournalTxBlock
{
    Guid TxId;
    unsigned int Type;
    unsigned char Unused[PageSize - 16 - 8 - 4];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalTxBlock) == PageSize, "Bad size");

struct JournalTxBeginBlock
{
    Guid TxId;
    unsigned int Type;
    unsigned char Unused[PageSize - 16 - 8 - 4];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalTxBeginBlock) == PageSize, "Bad size");

struct JournalTxDataBlock
{
    Guid TxId;
    unsigned int Type;
    unsigned int Index;
    unsigned int DataSize;
    unsigned long long Position;
    unsigned char Data[PageSize - 16 - 2 * 8 - 3 * 4];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalTxDataBlock) == PageSize, "Bad size");

struct JournalTxCommitBlock
{
    Guid TxId;
    unsigned int Type;
    unsigned int State;
    unsigned long long Time;
    unsigned char Unused[PageSize - 16 - 2 * 8 - 2 * 4];
    unsigned char Hash[HashSize];
};

static_assert(sizeof(JournalTxCommitBlock) == PageSize, "Bad size");

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