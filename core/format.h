#pragma once

namespace Core
{

class Format
{
public:
    static inline const char *TruncateFileName(const char *fileName)
    {
        const char *base, *lastSep = nullptr;

        base = fileName;
        for (;;)
        {
            if (*base == '\0')
                break;

            if (*base == '/')
            {
                lastSep = base;
            }
            base++;
        }

        if (lastSep)
            return lastSep + 1;
        else
            return fileName;
    }
};

}