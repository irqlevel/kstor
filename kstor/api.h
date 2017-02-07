#pragma once

namespace KStor 
{

namespace Api 
{

const unsigned int PacketMagic = 0xCCBECCBE;
const unsigned int PacketMaxDataSize = 65536;
const unsigned int GuidSize = 16;

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

#pragma pack(pop)

const unsigned int PacketTypePing = 1;

}

}