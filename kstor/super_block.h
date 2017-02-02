#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>

namespace KStor 
{

class SuperBlock
{
public:
    SuperBlock(const Core::AString& deviceName, bool format, Core::Error& err);
    virtual ~SuperBlock();

    Core::Error Format();
    Core::Error Load();

    unsigned long GetId() const;
    unsigned long long GetSize() const;

    const Core::AString& GetName() const;

    Core::BlockDevice& GetBDev();

private:
    Core::AString DeviceName;
    Core::BlockDevice BDev;
};

typedef Core::SharedPtr<SuperBlock, Core::Memory::PoolType::Kernel> SuperBlockRef;

}