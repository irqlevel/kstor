#pragma once

#include <kstor/api.h>

namespace KStor
{

namespace Control
{

class Ctl
{
public:
    Ctl(int& err);

    int GetTime(unsigned long long& time);
    int GetRandomUlong(unsigned long& value);

    int Mount(const char* deviceName, bool format, KStor::Api::Guid& volumeId);
    int Unmount(const KStor::Api::Guid& volumeId);
    int Unmount(const char* deviceName);

    int StartServer(const char *host, unsigned short port);
    int StopServer();

    int Test(unsigned int testId);

    int GetTaskStack(int pid, char *buf, unsigned long len);

    virtual ~Ctl();
private:
    int DevFd;
};

}

}