#pragma once

namespace KStor 
{

namespace Api 
{

const unsigned int PacketMagic = 0xCCBECCBE;
const unsigned int PacketMaxDataSize = 65536;
const unsigned int GuidSize = 16;
const unsigned int VolumeMagic = 0xCBDACBDA;

const unsigned int PacketTypePing = 1;

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

#pragma pack(pop)

}

}