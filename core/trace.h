#pragma once

namespace Core
{

class Trace
{
public:
    static void Output(int level, const char *fmt, ...);
    static void SetLevel(int level);
    static int GetLevel();

    static const char *TruncateFileName(const char *fileName);

private:
    static int Level;
};

}

#define trace(level, fmt, ...)                              \
              Core::Trace::Output(level, "%d: %s(),%s,%d: " fmt,   \
                            level, __FUNCTION__, Core::Trace::TruncateFileName(__FILE__), __LINE__, ##__VA_ARGS__)
