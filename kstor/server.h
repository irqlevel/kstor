#pragma once

#include <core/block_device.h>
#include <core/error.h>
#include <core/memory.h>
#include <core/shared_ptr.h>
#include <core/astring.h>
#include <core/socket.h>
#include <core/thread.h>
#include <core/unique_ptr.h>
#include <core/rwsem.h>
#include <core/auto_lock.h>
#include <core/runnable.h>
#include <core/threadable.h>
#include <core/list.h>

class Server : public Runnable
{
public:
    Server();
    virtual ~Server();
    Error Start(const AString &host, unsigned short port);
    void Stop();
private:
    class Connection : public Runnable {
    public:
        Connection(Server& srv, UniquePtr<Socket>&& sock, Error &err);
        void Stop();
        virtual ~Connection();
    private:
        Error Run(const Threadable& thread) override;
        Server& Srv;
        UniquePtr<Socket> Sock;
        UniquePtr<Thread> TransThread;
        RWSem StateLock;
    };

    typedef SharedPtr<Connection, Memory::PoolType::Kernel> ConnectionPtr;

    Error Run(const Threadable& thread) override;
    RWSem ConnListLock;
    RWSem StateLock;
    UniquePtr<Socket> ListenSocket;
    UniquePtr<Thread> AcceptThread;
    LinkedList<ConnectionPtr, Memory::PoolType::Kernel> ConnList;
};