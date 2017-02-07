#include "guid.h"


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

}