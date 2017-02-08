#include "guid.h"
#include <core/hex.h>

namespace KStor
{

Guid::Guid()
{
    Core::Memory::MemSet(Content.Data, 0, sizeof(Content.Data));
}

Guid::Guid(Core::Random& rng, Core::Error& err)
    : Guid()
{
    if (!err.Ok())
        return;
    
    err = Generate(rng);
}

Guid::Guid(Core::Error& err)
    : Guid()
{
    if (!err.Ok())
        return;

    err = Generate();
}

Guid::Guid(const Api::Guid& content)
    : Content(content)
{
}

Core::Error Guid::Generate(Core::Random& rng)
{
    return rng.GetBytes(Content.Data, sizeof(Content.Data));
}

Core::Error Guid::Generate()
{
    Core::Error err;

    Core::Random rng(err, false);
    if (!err.Ok())
        return err;

    return rng.GetBytes(Content.Data, sizeof(Content.Data));
}

const Api::Guid& Guid::GetContent() const
{
    return Content;
}

void Guid::SetContent(const Api::Guid& content)
{
    Content = content;
}

bool Guid::operator==(const Guid& other) const
{
    return (Core::Memory::MemCmp(const_cast<unsigned char*>(Content.Data),
            const_cast<unsigned char*>(other.Content.Data), sizeof(Content.Data)) == 0) ? true : false;
}

Guid::~Guid()
{
}

Core::AString Guid::ToString() const
{
    return Core::Hex::Encode(Content.Data, sizeof(Content.Data));
}

Guid::Guid(const Guid& other)
{
    Content = other.GetContent();
}

Guid::Guid(Guid&& other)
{
    Content = other.GetContent();
    other.Clear();
}

Guid& Guid::operator=(const Guid& other)
{
    if (this != &other)
    {
        Content = other.GetContent();
    }
    return *this;
}

Guid& Guid::operator=(Guid&& other)
{
    if (this != &other)
    {
        Content = other.GetContent();
        other.Clear();
    }
    return *this;
}

void Guid::Clear()
{
    Core::Memory::MemSet(Content.Data, 0, sizeof(Content.Data));
}

size_t Guid::Hash() const 
{
    size_t hash = 5381;
    size_t c;

    for (size_t i = 0; i < sizeof(Content.Data); i++)
    {
        c = Content.Data[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

}