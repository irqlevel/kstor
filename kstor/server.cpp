#include "server.h"
#include "api.h"

namespace KStor 
{

Server::Server()
{
}

Server::Connection::Connection(Server& srv, Core::UniquePtr<Core::Socket>&& socket)
    : Srv(srv), Sock(Core::Memory::Move(socket))
{
    trace(1, "Connection 0x%p created", this);
}

Core::Error Server::Connection::Start()
{
    trace(1, "Connection 0x%p starting", this);

    Core::AutoLock lock(StateLock);
    if (ConnThread.Get() != nullptr || Sock.Get() == nullptr)
    {
        return Core::Error::InvalidState;
    }

    Core::Error err;
    ConnThread.Reset(new Core::Thread(this, err));
    if (ConnThread.Get() == nullptr)
    {
        err.SetNoMemory();
        return err;
    }
    if (!err.Ok())
    {
        ConnThread.Reset();
        return err;
    }
    return err;
}

void Server::Connection::Stop()
{
    trace(1, "Connection 0x%p stopping", this);

    Core::AutoLock lock(StateLock);

    ConnThread.Reset();
    Sock.Reset();
}

Server::Connection::~Connection()
{
    Stop();
}

Core::Error Server::Connection::Run(const Core::Threadable& thread)
{
    trace(1, "Connection 0x%p thread", this);

    while (!thread.IsStopping())
    {
        Core::Thread::Sleep(10);
    }

    trace(1, "Connection 0x%p thread exiting", this);

    return Core::Error::Success;
}

Core::Error Server::Run(const Core::Threadable &thread)
{
    Core::Error err;

    trace(1, "Server 0x%p thread", this);

    for (;;)
    {
        {
            Core::AutoLock lock(ConnListLock);
            if (thread.IsStopping())
                break;
        }

        if (ListenSocket.Get() != nullptr)
        {
            Core::UniquePtr<Core::Socket> socket(ListenSocket->Accept(err));
            if (socket.Get() == nullptr || !err.Ok())
            {
                trace(0, "Server 0x%p socket accept error %d", this, err.GetCode());
                continue;
            }

            ConnectionPtr conn(new (Core::Memory::PoolType::Kernel) Connection(*this, Core::Memory::Move(socket)));
            if (conn.Get() == nullptr)
            {
                err.SetNoMemory();
            }

            if (!err.Ok())
            {
                trace(0, "Server 0x%p create connection error %d", this, err.GetCode());
                continue;
            }

            {
                Core::AutoLock lock(ConnListLock);
                if (!thread.IsStopping())
                {
                    if (!ConnList.AddTail(conn))
                    {
                        err.SetNoMemory();
                        trace(0, "Server 0x%p can't insert connection err %d", this, err.GetCode());
                        continue;
                    }
                    err = conn->Start();
                    if (!err.Ok())
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

    return Core::Error::Success;
}

Core::Error Server::Start(const Core::AString& host, unsigned short port)
{
    trace(1, "Server 0x%p starting", this);

    Core::AutoLock lock(StateLock);

    if (ListenSocket.Get() != nullptr || AcceptThread.Get() != nullptr)
        return Core::Error::InvalidState;

    ListenSocket.Reset(new (Core::Memory::PoolType::Kernel) Core::Socket());
    if (ListenSocket.Get() == nullptr)
        return Core::Error::NoMemory;

    Core::Error err = ListenSocket->Listen(host, port);
    if (!err.Ok()) {
        ListenSocket.Reset();
        return err;
    }

    AcceptThread.Reset(new (Core::Memory::PoolType::Kernel) Core::Thread(this, err));
    if (AcceptThread.Get() == nullptr) {
        ListenSocket.Reset();
        return Core::Error::NoMemory;
    }

    if (!err.Ok()) {
        AcceptThread.Reset();
        ListenSocket.Reset();
        return err;
    }

    trace(1, "Server 0x%p started", this);

    return Core::Error::Success;
}

void Server::Stop()
{
    trace(1, "Server 0x%p stopping", this);

    Core::AutoLock lock(StateLock);
    {
        Core::AutoLock lock(ConnListLock);
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
        Core::AutoLock lock(ConnListLock);
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

}