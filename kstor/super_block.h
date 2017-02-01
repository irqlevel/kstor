#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>

class SuperBlock
{
public:
    SuperBlock(const AString& deviceName, bool format, Error& err);
    virtual ~SuperBlock();

    Error Format();
    Error Load();

    unsigned long GetId() const;
    unsigned long long GetSize() const;

    const AString& GetName() const;

    BlockDevice& GetBDev();

private:
    AString DeviceName;
    BlockDevice BDev;
};

typedef SharedPtr<SuperBlock, Memory::PoolType::Kernel> SuperBlockRef;