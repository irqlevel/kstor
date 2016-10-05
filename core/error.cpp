#include "error.h"

Error::Error()
    : Code(Success)
{
}

Error::Error(int code)
    : Code(code)
{
}

Error::~Error()
{
}

int Error::GetCode() const
{
    return Code;
}

void Error::SetCode(int code)
{
    Code = code;
}

const char* Error::GetDescription() const
{
    switch (Code)
    {
    case Success:
        return "Success";
    case InvalidValue:
        return "Invalid value";
    case NoMemory:
        return "No memory";
    case Cancelled:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

bool Error::operator!= (const Error& other) const
{
    return GetCode() != other.GetCode();
}

bool Error::operator== (const Error& other) const
{
    return GetCode() == other.GetCode();
}
