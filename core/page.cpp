#include "page.h"
#include "kapi.h"

namespace Core
{

PageMap::PageMap(PageInterface& page)
    : PageRef(page)
    , Address(nullptr)
{
    Address = PageRef.Map();
}

PageMap::~PageMap()
{
    Unmap();
}

void* PageMap::GetAddress()
{
    return Address;
}

void PageMap::Unmap()
{
    if (Address != nullptr)
    {
        PageRef.Unmap();
        Address = nullptr;
    }
}

}