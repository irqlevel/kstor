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
#include <core/vector.h>

#include "api.h"

namespace KStor 
{

class Packet
{
public:
    using Ptr = Core::SharedPtr<Packet>;

    Packet();
    Packet(const Api::PacketHeader &header, Core::Error &err);
    Core::Error Parse(const Api::PacketHeader &header);
    Core::Error Create(unsigned int type, unsigned int result, unsigned int dataSize);

    void PrepareSend();

    virtual ~Packet();
    unsigned int GetType() const;
    unsigned int GetResult() const;
    size_t GetDataSize() const;
    size_t GetSize() const;
    void* GetBody();
    void* GetData();

    void SetResult(unsigned int result);

private:
    Api::PacketHeader *GetHeader();
    unsigned int Type;
    unsigned int Result;
    unsigned int DataSize;
    Core::Vector<unsigned char> Body;
};

class Server : public Core::Runnable
{
public:
    Server();
    virtual ~Server();
    Core::Error Start(const Core::AString &host, unsigned short port);
    void Stop();
private:
    class Connection : public Core::Runnable {
    public:
        using Ptr = Core::SharedPtr<Connection>;

        Connection(Server& srv, Core::UniquePtr<Core::Socket>&& sock);
        Core::Error Start();
        void Stop();
        virtual ~Connection();

        Packet::Ptr RecvPacket(Core::Error& err);
        Core::Error SendPacket(Packet::Ptr& packet);

        bool Closed();

    private:
        Core::Error Run(const Core::Threadable& thread) override;
        Server& Srv;
        Core::UniquePtr<Core::Socket> Sock;
        Core::UniquePtr<Core::Thread> ConnThread;
        Core::RWSem StateLock;
    };

    Core::Error Run(const Core::Threadable& thread) override;
    Core::RWSem ConnListLock;
    Core::RWSem StateLock;
    Core::UniquePtr<Core::Socket> ListenSocket;
    Core::UniquePtr<Core::Thread> AcceptThread;
    Core::LinkedList<Connection::Ptr> ConnList;

    Core::Error HandleChunkCreate(Packet::Ptr& request, Packet::Ptr& response);
    Core::Error HandleChunkWrite(Packet::Ptr& request, Packet::Ptr& response);
    Core::Error HandleChunkRead(Packet::Ptr& request, Packet::Ptr& response);
    Core::Error HandleChunkDelete(Packet::Ptr& request, Packet::Ptr& response);

    Core::Error HandlePing(Packet::Ptr& request, Packet::Ptr& response);

    Packet::Ptr HandleRequest(Packet::Ptr& request, Core::Error& err);

};

}