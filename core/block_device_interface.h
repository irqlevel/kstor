#pragma once

class BlockDeviceInterface
{
public:
    virtual void* GetBdev() = 0;
};