#pragma once

#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/random.h>
#include <core/astring.h>

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
    Guid(const Guid& other);
    Guid(Guid&& other);
    Guid& operator=(const Guid& other);
    Guid& operator=(Guid&& other);

    const Api::Guid& GetContent() const;

    Core::Error Generate(Core::Random& rng);
    Core::Error Generate();

    void Clear();

    void SetContent(const Api::Guid& content);

    virtual ~Guid();

    bool operator==(const Guid& other) const;

    Core::AString ToString() const;

private:
    Api::Guid Content;
};

typedef Core::SharedPtr<Guid, Core::Memory::PoolType::Kernel> GuidPtr;

}