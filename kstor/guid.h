#pragma once

#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/random_file.h>
#include <core/astring.h>

#include "api.h"

namespace KStor
{

class Guid
{
public:
    using Ptr = Core::SharedPtr<Guid>;

    Guid();
    Guid(Core::RandomFile& rng, Core::Error& err);
    Guid(Core::Error& err);
    Guid(const Api::Guid& content);
    Guid(const Guid& other);
    Guid(Guid&& other);
    Guid& operator=(const Guid& other);
    Guid& operator=(Guid&& other);

    const Api::Guid& GetContent() const;

    Core::Error Generate(Core::RandomFile& rng);
    Core::Error Generate();

    void Clear();

    void SetContent(const Api::Guid& content);

    virtual ~Guid();

    bool operator==(const Guid& other) const;
    bool operator!=(const Guid& other) const;

    Core::AString ToString() const;

    size_t Hash() const;

private:
    int Compare(const Guid& other) const;

    Api::Guid Content;
};

}