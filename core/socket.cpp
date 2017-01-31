#include "socket.h"

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

    return get_kapi()->sock_connect(&Sockp, const_cast<char *>(host.GetBuf()), port);
}

Error Socket::Listen(const AString& host, unsigned short port)
{
    if (Sockp != nullptr)
        return Error::InvalidState;

    return get_kapi()->sock_listen(&Sockp, const_cast<char *>(host.GetBuf()), port, 0);
}

Socket *Socket::Accept(Error &err)
{
    if (Sockp == nullptr) {
        err = Error::InvalidState;
        return nullptr;
    }

    void *newSockp = nullptr;
    err = get_kapi()->sock_accept(&newSockp, Sockp);
    if (err != Error::Success)
        return nullptr;

    Socket *newSock = new (Memory::PoolType::Kernel) Socket(newSockp);
    if (!newSock)
        err = Error::NoMemory;

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

Error Socket::Send(void *buf, unsigned long len, unsigned long& sent)
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
    if (sent < len)
        return Error::EOF;
    
    return Error::Success;
}

Error Socket::SendAll(void *buf, unsigned long len, unsigned long& sent)
{
    Error err;

    sent = 0;
    while (sent < len) {
        unsigned long lsent;

        err = Send(Memory::MemAdd(buf, sent), len - sent, lsent);
        if (err != Error::Success && err != Error::EOF) {
            break;
        }

        if (sent == 0) {
            err = Error::EOF;
            break;
        }

        err = Error::Success;
        sent += lsent;
    }
    return err;
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
    if (recv < len)
        return Error::EOF;

    return Error::Success;
}

Error Socket::RecvAll(void *buf, unsigned long len, unsigned long& recv)
{
    Error err;

    recv = 0;
    while (recv < len) {
        unsigned long lrecv;

        err = Send(Memory::MemAdd(buf, recv), len - recv, lrecv);
        if (err != Error::Success && err != Error::EOF) {
            break;
        }

        if (lrecv == 0) {
            err = Error::EOF;
            break;
        }

        err = Error::Success;
        recv += lrecv;
    }
    return err;
}

Socket::~Socket()
{
    Close();
}