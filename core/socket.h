#pragma once

#include "kapi.h"
#include "error.h"
#include "astring.h"

namespace Core
{

class Socket
{
public:
    Socket();
    Error Connect(const AString& host, unsigned short port);
    Error Listen(const AString& host, unsigned short port, int backlog);
    Socket *Accept(Error &err);
    Error Send(void *buf, unsigned long len, unsigned long& sent);
    Error SendAll(void *buf, unsigned long len, unsigned long& sent);
    Error Recv(void *buf, unsigned long len, unsigned long& recv);
    Error RecvAll(void *buf, unsigned long len, unsigned long& recv);
    void AbortAccept();
    void Close();    
    virtual ~Socket();
private:
    Socket(void *sockp);
    void *Sockp;
};

}