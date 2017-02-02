#pragma once

namespace Core
{

class Threadable
{
public:
    virtual bool IsStopping() const = 0;
    virtual void* GetId() const = 0;
};

}
