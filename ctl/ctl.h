#pragma once

class KStorCtl
{
public:
    KStorCtl(int& err);

    int GetTime(unsigned long long& time);
    int GetRandomUlong(unsigned long& value);

    int Mount(const char* deviceName, bool format, unsigned long& deviceId);
    int Unmount(unsigned long& deviceId);
    int Unmount(const char* deviceName);

    virtual ~KStorCtl();
private:
    int DevFd;
};