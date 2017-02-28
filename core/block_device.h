#pragma once

#include "error.h"
#include "astring.h"
#include "page.h"
#include "bio.h"
#include "block_device_interface.h"

namespace Core
{

class BlockDevice : public BlockDeviceInterface
{
public:
    BlockDevice(const AString& deviceName, Error& err);
    virtual void* GetBdev() override;
    unsigned long long GetSize() const;
    virtual ~BlockDevice();

    template<Memory::PoolType PoolType>
    Core::Error ExecIo(const typename Page<PoolType>::Ptr& page, unsigned long long position, bool write)
    {
        if (position & 511)
            return Error::InvalidValue;

        Error err;
        Bio<PoolType> bio(*this, page, position / 512, err, write);
        if (!err.Ok())
        {
            trace(0, "BDev 0x%p can't init bio, err %d", this, err.GetCode());
            return err;
        }

        return bio.Exec();
    }

    template<Memory::PoolType PoolType>
    Core::Error Write(const typename Page<PoolType>::Ptr& page, unsigned long long position)
    {
        return ExecIo<PoolType>(page, position, true);
    }

    template<Memory::PoolType PoolType>
    Core::Error Read(const typename Page<PoolType>::Ptr& page, unsigned long long position)
    {
        return ExecIo<PoolType>(page, position, false);
    }

private:
    void* BDevPtr;
    int Mode;
};

}