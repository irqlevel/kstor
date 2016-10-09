#pragma once

class KStorCtl
{
public:
    KStorCtl(int& err);
    int GetTime(unsigned long long& time);
    int GetRandomUlong(unsigned long& value);

    int DeviceAdd(const char* deviceName, bool format, unsigned long& deviceId);
    int DeviceRemove(unsigned long& deviceId);

    virtual ~KStorCtl();
private:
    int DevFd;
};