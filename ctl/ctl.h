#pragma once

class KStorCtl
{
public:
    KStorCtl(int& err);
    int GetTime(unsigned long long& time);
    virtual ~KStorCtl();
private:
    int DevFd;
};