#pragma once

#include "astring.h"

namespace Core
{

class Hex
{
private:
    static char ByteToHexChar(unsigned char val)
    {
        if (val >= 16)
            return '?';

        if (val < 10)
            return '0' + val;
        else
            return 'a' + val - 10;
    }

public:
    static Core::AString Encode(const unsigned char *buf, size_t len)
    {
        Core::AString result;

        if (!result.ReserveAndUse(2 * len))
            return result;
        
        char* dst = const_cast<char *>(result.GetBuf());
        for (size_t i = 0; i < len; i++)
        {
            char c = buf[i];
            dst[2 * i] = ByteToHexChar((c >> 4) & 0xF);
            dst[2 * i + 1] = ByteToHexChar(c & 0xF);
        }
        dst[2 * len] = '\0';

        return result;
    }

};

}