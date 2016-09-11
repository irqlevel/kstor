#include "bio.h"

Bio::Bio(Error& err)
    : BioPtr(nullptr)
{
    if (err != Error::Success)
    {
        return;
    }
}

Bio::~Bio()
{
}