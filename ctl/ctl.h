#pragma once

class KStorCtl
{
public:
    KStorCtl(int& err);
    int GetTime(unsigned long long& time);
    int GetRandomUlong(unsigned long& value);
    virtual ~KStorCtl();
private:
    int DevFd;
};