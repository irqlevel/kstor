#pragma once

#include "main.h"
#include "vector.h"
#include "shared_ptr.h"

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
    size_t Hash() const;

    static int Compare(const AString& key1, const AString& key2);
    static size_t Hash(const AString& key);

private:
    AString(const AString& other) = delete;
    AString& operator=(const AString& other) = delete;
    Vector<char> Buf;
};

typedef shared_ptr<AString> AStringRef;
