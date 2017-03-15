#include "socket.h"

namespace Core
{

Socket::Socket()
    : Socket(nullptr)
{
}

Socket::Socket(void *sockp)
    : Sockp(sockp)
{
}

Error Socket::Connect(const AString& host, unsigned short port)
{
    if (Sockp != nullptr)
        return Error::InvalidState;

    return get_kapi()->sock_connect(&Sockp, const_cast<char *>(host.GetConstBuf()), port);
}

Error Socket::Listen(const AString& host, unsigned short port, int backlog)
{
    if (Sockp != nullptr)
        return Error::InvalidState;

    return get_kapi()->sock_listen(&Sockp, const_cast<char *>(host.GetConstBuf()), port, backlog);
}

Socket *Socket::Accept(Error &err)
{
    if (Sockp == nullptr) {
        err.SetInvalidState();
        return nullptr;
    }

    void *newSockp = nullptr;
    err = get_kapi()->sock_accept(&newSockp, Sockp);
    if (!err.Ok())
        return nullptr;

    Socket *newSock = new (Memory::PoolType::Kernel) Socket(newSockp);
    if (!newSock)
        err.SetNoMemory();

    return newSock;
}

void Socket::AbortAccept()
{
    if (Sockp != nullptr)
        get_kapi()->sock_abort_accept(Sockp);
}

void Socket::Close()
{
    if (Sockp != nullptr) {
        get_kapi()->sock_release(Sockp);
        Sockp = nullptr;
    }
}

Error Socket::Send(const void *buf, unsigned long len, unsigned long& sent)
{
    sent = 0;
    if (Sockp == nullptr)
        return Error::InvalidState;

    if (len > Memory::MaxInt)
        return Error::BufToBig;

    int r = get_kapi()->sock_send(Sockp, buf, static_cast<int>(len));
    if (r < 0)
        return Error(r);

    sent = static_cast<unsigned long>(r);
    return Error::Success;
}

Error Socket::Recv(void *buf, unsigned long len, unsigned long& recv)
{
    recv = 0;
    if (Sockp == nullptr)
        return Error::InvalidState;

    if (len > Memory::MaxInt)
        return Error::BufToBig;

    int r = get_kapi()->sock_recv(Sockp, buf, static_cast<int>(len));
    if (r < 0)
        return Error(r);

    recv = static_cast<unsigned long>(r);

    return Error::Success;
}

Error Socket::SendAll(const void *buf, unsigned long len, unsigned long& sent)
{
    Error err;

    sent = 0;
    while (sent < len) {
        unsigned long lsent;

        err = Send(Memory::MemAdd(buf, sent), len - sent, lsent);
        sent += lsent;
        if (!err.Ok())
        {
            if (err == Core::Error::Again)
                continue;

            break;
        }

        if (lsent == 0) {
            err.SetEOF();
            break;
        }
    }
    return err;
}

Error Socket::RecvAll(void *buf, unsigned long len, unsigned long& recv)
{
    Error err;

    recv = 0;
    while (recv < len) {
        unsigned long lrecv;

        err = Recv(Memory::MemAdd(buf, recv), len - recv, lrecv);
        recv += lrecv;
        if (!err.Ok())
        {
            if (err == Core::Error::Again)
                continue;

            break;
        }

        if (lrecv == 0) {
            err.SetEOF();
            break;
        }
    }
    return err;
}

Socket::~Socket()
{
    Close();
}

}