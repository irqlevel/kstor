#pragma once

#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/random.h>

#include "api.h"

namespace KStor
{

class Guid
{
public:
    Guid();
    Guid(Core::Random& rng, Core::Error& err);
    Guid(Core::Error& err);
    Guid(const Api::Guid& content);

    const Api::Guid& GetContent() const;

    Core::Error Generate(Core::Random& rng);
    Core::Error Generate();

    void SetContent(const Api::Guid& content);

    virtual ~Guid();

    bool operator==(const Guid& other) const;

private:
    Api::Guid Content;
};

typedef Core::SharedPtr<Guid, Core::Memory::PoolType::Kernel> GuidPtr;

}