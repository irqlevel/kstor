#pragma once

#include "main.h"
#include "vector.h"

class AString
{
public:
    AString(const char* s, MemType memType, int& err);
    virtual ~AString();

    const char* GetBuf() const;
    size_t GetLen() const;

    AString(AString&& other);
    AString& operator=(AString&& other);

    int Compare(const AString& other) const;

private:
    AString(const AString& other) = delete;
    AString& operator=(const AString& other) = delete;
    Vector<char> Buf;
};
