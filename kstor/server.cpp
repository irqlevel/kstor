#include "server.h"

Server::Server()
{
}

Server::Connection::Connection(Server& srv, UniquePtr<Socket>&& socket)
    : Srv(srv), Sock(Memory::Move(socket))
{
    trace(1, "Connection 0x%p created", this);
}

Error Server::Connection::Start()
{
    trace(1, "Connection 0x%p starting", this);

    AutoLock lock(StateLock);
    if (TransThread.Get() != nullptr || Sock.Get() == nullptr)
    {
        return Error::InvalidState;
    }

    Error err;
    TransThread.Reset(new Thread(this, err));
    if (TransThread.Get() == nullptr)
    {
        return Error::NoMemory;
    }

    return Error::Success;
}

void Server::Connection::Stop()
{
    trace(1, "Connection 0x%p stopping", this);

    AutoLock lock(StateLock);

    TransThread.Reset();
    Sock.Reset();
}

Server::Connection::~Connection()
{
    Stop();
}

Error Server::Connection::Run(const Threadable& thread)
{
    trace(1, "Connection 0x%p thread", this);

    while (!thread.IsStopping())
    {
        Thread::Sleep(10);
        trace(1, "Connection 0x%p thread", this);
    }

    trace(1, "Connection 0x%p thread exiting", this);

    return Error::Success;
}

Error Server::Run(const Threadable &thread)
{
    Error err;

    trace(1, "Server 0x%p thread", this);

    for (;;)
    {
        {
            AutoLock lock(ConnListLock);
            if (thread.IsStopping())
                break;
        }

        if (ListenSocket.Get() != nullptr)
        {
            UniquePtr<Socket> socket(ListenSocket->Accept(err));
            if (socket.Get() == nullptr || err != Error::Success)
            {
                trace(0, "Server 0x%p socket accept error %d", this, err.GetCode());
                continue;
            }

            ConnectionPtr conn(new (Memory::PoolType::Kernel) Connection(*this, Memory::Move(socket)));
            if (conn.Get() == nullptr)
            {
                err = Error::NoMemory;
            }

            if (err != Error::Success)
            {
                trace(0, "Server 0x%p create connection error %d", this, err.GetCode());
                continue;
            }

            {
                AutoLock lock(ConnListLock);
                if (!thread.IsStopping())
                {
                    if (!ConnList.AddTail(conn))
                    {
                        err = Error::NoMemory;
                        trace(0, "Server 0x%p can't insert connection err %d", this, err.GetCode());
                        continue;
                    }
                    err = conn->Start();
                    if (err != Error::Success)
                    {
                        trace(0, "Server 0x%p can't start connection err %d", this, err.GetCode());
                        ConnList.PopTail();
                        continue;
                    }
                }
            }
        }
    }

    trace(1, "Server 0x%p thread exiting", this);

    return Error::Success;
}

Error Server::Start(const AString& host, unsigned short port)
{
    trace(1, "Server 0x%p starting", this);

    AutoLock lock(StateLock);

    if (ListenSocket.Get() != nullptr || AcceptThread.Get() != nullptr)
        return Error::InvalidState;

    ListenSocket.Reset(new (Memory::PoolType::Kernel) Socket());
    if (ListenSocket.Get() == nullptr)
        return Error::NoMemory;

    Error err = ListenSocket->Listen(host, port);
    if (err != Error::Success) {
        ListenSocket.Reset();
        return err;
    }

    AcceptThread.Reset(new (Memory::PoolType::Kernel) Thread(this, err));
    if (AcceptThread.Get() == nullptr) {
        ListenSocket.Reset();
        return Error::NoMemory;
    }

    if (err != Error::Success) {
        AcceptThread.Reset();
        ListenSocket.Reset();
        return err;
    }

    trace(1, "Server 0x%p started", this);

    return Error::Success;
}

void Server::Stop()
{
    trace(1, "Server 0x%p stopping", this);

    AutoLock lock(StateLock);
    {
        AutoLock lock(ConnListLock);
        if (AcceptThread.Get() != nullptr)
        {
            AcceptThread->Stop();
        }
    }

    if (ListenSocket.Get() != nullptr)
    {
        ListenSocket->AbortAccept();
    }

    AcceptThread.Reset();
    ListenSocket.Reset();

    {
        AutoLock lock(ConnListLock);
        auto it = ConnList.GetIterator();
        for (;it.IsValid(); it.Next())
        {
            ConnectionPtr& conn = it.Get();
            conn->Stop();
        }
    }

    trace(1, "Server 0x%p stopped", this);
}

Server::~Server()
{
    Stop();
}