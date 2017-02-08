#include "server.h"
#include "control_device.h"
#include <core/bitops.h>

namespace KStor 
{

Packet::Packet()
    : Type(0), Result(0), DataSize(0)
{
}

Packet::Packet(const Api::PacketHeader &header, Core::Error &err)
    : Packet()
{
    if (!err.Ok())
        return;

    err = Parse(header);
}

Api::PacketHeader *Packet::GetHeader()
{
    return reinterpret_cast<Api::PacketHeader*>(const_cast<unsigned char *>(Body.GetBuf()));
}

Core::Error Packet::Parse(const Api::PacketHeader &header)
{
    unsigned int magic = Core::BitOps::Le32ToCpu(header.Magic);
    unsigned int type = Core::BitOps::Le32ToCpu(header.Type);
    unsigned int result = Core::BitOps::Le32ToCpu(header.Result);
    unsigned int dataSize = Core::BitOps::Le32ToCpu(header.DataSize);

    if (magic != Api::PacketMagic)
        return Core::Error::InvalidValue;
    if (dataSize > Api::PacketMaxDataSize)
        return Core::Error::InvalidValue;

    Body.Clear();
    if (!Body.ReserveAndUse(dataSize + sizeof(Api::PacketHeader)))
        return Core::Error::NoMemory;

    Type = type;
    Result = result;
    DataSize = dataSize;

    return Core::Error::Success;
}

size_t Packet::GetDataSize() const
{
    return DataSize;
}

size_t Packet::GetSize() const
{
    return Body.GetSize();
}

unsigned int Packet::GetType() const
{
    return Type;
}

unsigned int Packet::GetResult() const
{
    return Result;
}

void Packet::SetResult(unsigned int result)
{
    Result = result;
    GetHeader()->Result = Core::BitOps::CpuToLe32(Result);
}

void* Packet::GetData()
{
    return Core::Memory::MemAdd(GetBody(), sizeof(Api::PacketHeader));
}

void* Packet::GetBody()
{
    return const_cast<unsigned char *>(Body.GetBuf());
}

Core::Error Packet::Prepare(unsigned int type, unsigned int result, unsigned int dataSize)
{
    if (dataSize > Api::PacketMaxDataSize)
        return Core::Error::BufToBig;

    Body.Clear();
    if (!Body.ReserveAndUse(dataSize + sizeof(Api::PacketHeader)))
        return Core::Error::NoMemory;

    Type = type;
    Result = result;
    DataSize = dataSize;

    GetHeader()->Type = Core::BitOps::CpuToLe32(Type);
    GetHeader()->Result = Core::BitOps::CpuToLe32(Result);
    GetHeader()->DataSize = Core::BitOps::CpuToLe32(DataSize);
    GetHeader()->Magic = Core::BitOps::CpuToLe32(Api::PacketMagic);
    return Core::Error::Success;
}

Packet::~Packet()
{
}

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
    ConnThread = Core::MakeUnique<Core::Thread, Core::Memory::PoolType::Kernel>(this, err);
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
    if (Sock.Get() != nullptr)
    {
        Sock->Close();
        Sock.Reset();
    }
}

bool Server::Connection::Closed()
{
    Core::AutoLock lock(StateLock);
    return (Sock.Get() == nullptr) ? true : false;
}

Server::Connection::~Connection()
{
    Stop();
    trace(1, "Connection 0x%p dtor", this);
}

PacketPtr Server::Connection::RecvPacket(Core::Error& err)
{
    PacketPtr packet;
    Api::PacketHeader header;
    unsigned long received;

    err = Sock->RecvAll(&header, sizeof(header), received);
    if (!err.Ok())
    {
        trace(1, "Connection 0x%p read header err %d", this, err.GetCode());
        return packet;
    }

    packet = Core::MakeShared<Packet, Core::Memory::PoolType::Kernel>(header, err);
    if (packet.Get() == nullptr)
    {
        err.SetNoMemory();
        trace(1, "Connection 0x%p can't allocate packet err %d", this, err.GetCode());
        packet.Reset();
        return packet;
    }

    if (!err.Ok())
    {
        trace(1, "Connection 0x%p can't parse packet err %d", this, err.GetCode());
        packet.Reset();
        return packet;
    }

    err = Sock->RecvAll(packet->GetData(), packet->GetDataSize(), received);
    if (!err.Ok())
    {
        trace(1, "Connection 0x%p can't read packet body err %d", this, err.GetCode());
        packet.Reset();
        return packet;
    }

    return packet;
}

Core::Error Server::Connection::SendPacket(PacketPtr& packet)
{
    unsigned long sent;
    Core::Error err = Sock->SendAll(packet->GetBody(), packet->GetSize(), sent);
    if (!err.Ok())
    {
        trace(1, "Connection 0x%p can't send packet err %d", this, err.GetCode());
        return err;
    }

    trace(1, "Connection 0x%p sent packet size %lu", this, sent);

    return err;
}

Core::Error Server::Connection::Run(const Core::Threadable& thread)
{
    Core::Error err;

    trace(1, "Connection 0x%p thread", this);

    while (!thread.IsStopping())
    {
        trace(1, "Connection 0x%p receiving request", this);
        auto request = RecvPacket(err);
        if (!err.Ok())
        {
            trace(1, "Connection 0x%p recv packet err %d", this, err.GetCode());
            break;
        }
        trace(1, "Connection 0x%p handling request", this);
        auto response = Srv.HandleRequest(request, err);
        if (!err.Ok())
        {
            trace(1, "Connection 0x%p handle packet err %d", this, err.GetCode());
            break;
        }
        trace(1, "Connection 0x%p sending response", this);
        err = SendPacket(response);
        if (!err.Ok())
        {
            trace(1, "Connection 0x%p send packet err %d", this, err.GetCode());
            break;
        }
    }

    trace(1, "Connection 0x%p thread closing socket", this);

    {
        Core::AutoLock lock(StateLock);
        if (Sock.Get() != nullptr)
        {
            Sock->Close();
            Sock.Reset();
        }
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

            auto it = ConnList.GetIterator();
            for(;it.IsValid();it.Next())
            {
                auto conn = it.Get();
                if (conn->Closed())
                {
                    trace(1, "Server 0x%p removing died connection 0x%p", this, conn.Get());
                    it.Erase();
                }
            }
        }

        if (ListenSocket.Get() == nullptr)
            break;

        Core::UniquePtr<Core::Socket> socket(ListenSocket->Accept(err));
        if (socket.Get() == nullptr || !err.Ok())
        {
            trace(0, "Server 0x%p socket accept error %d", this, err.GetCode());
            continue;
        }

        ConnectionPtr conn = Core::MakeShared<Connection, Core::Memory::PoolType::Kernel>(*this, Core::Memory::Move(socket));
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

    trace(1, "Server 0x%p thread exiting", this);

    return Core::Error::Success;
}

Core::Error Server::Start(const Core::AString& host, unsigned short port)
{
    trace(1, "Server 0x%p starting", this);

    Core::AutoLock lock(StateLock);

    if (ListenSocket.Get() != nullptr || AcceptThread.Get() != nullptr)
        return Core::Error::InvalidState;

    ListenSocket = Core::MakeUnique<Core::Socket, Core::Memory::PoolType::Kernel>();
    if (ListenSocket.Get() == nullptr)
        return Core::Error::NoMemory;

    Core::Error err = ListenSocket->Listen(host, port);
    if (!err.Ok()) {
        ListenSocket.Reset();
        return err;
    }

    AcceptThread = Core::MakeUnique<Core::Thread, Core::Memory::PoolType::Kernel>(this, err);
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

    for (;;)
    {
        if (ConnList.IsEmpty())
            break;

        ConnectionPtr conn;
        {
            Core::AutoLock lock(ConnListLock);
            if (ConnList.IsEmpty())
                break;

            auto it = ConnList.GetIterator();
            for (;it.IsValid(); it.Next())
            {
                conn = it.Get();
                it.Erase();
                break;
            }
        }

        if (conn.Get() != nullptr)
            conn->Stop();
    }

    trace(1, "Server 0x%p stopped", this);
}

Server::~Server()
{
    Stop();
}

Core::Error Server::HandlePing(PacketPtr& request, PacketPtr& response)
{
    Core::Error err;

    trace(1, "Server 0x%p handle ping, data size %lu",
        this, request->GetDataSize());

    err = response->Prepare(request->GetType(), Api::ResultSuccess, request->GetDataSize());
    if (!err.Ok())
        return err;

    Core::Memory::MemCpy(response->GetData(), request->GetData(), response->GetDataSize());

    return err;
}

Core::Error Server::HandleChunkCreate(PacketPtr& request, PacketPtr& response)
{
    Core::Error err;

    Api::ChunkCreateRequest* req = static_cast<Api::ChunkCreateRequest*>(request->GetData());
    if (request->GetDataSize() != sizeof(*req))
    {
        return response->Prepare(request->GetType(), Api::ResultUnexpectedDataSize, 0);
    }

    err =  response->Prepare(request->GetType(), Api::ResultSuccess, 0);
    if (!err.Ok())
        return err;

    err = ControlDevice::Get()->ChunkCreate(req->ChunkId);
    if (!err.Ok())
    {
        err.Clear();
        response->SetResult(Api::ResultNotFound);
    }

    return response->Prepare(request->GetType(), Api::ResultSuccess, 0);
}

Core::Error Server::HandleChunkWrite(PacketPtr& request, PacketPtr& response)
{
    Core::Error err;

    Api::ChunkWriteRequest* req = static_cast<Api::ChunkWriteRequest*>(request->GetData());
    if (request->GetDataSize() != sizeof(*req))
    {
        return response->Prepare(request->GetType(), Api::ResultUnexpectedDataSize, 0);
    }

    err = response->Prepare(request->GetType(), Api::ResultSuccess, 0);
    if (!err.Ok())
        return err;

    err = ControlDevice::Get()->ChunkWrite(req->ChunkId, req->Data);
    if (!err.Ok())
    {
        err.Clear();
        response->SetResult(Api::ResultNotFound);
    }

    return err;
}

Core::Error Server::HandleChunkRead(PacketPtr& request, PacketPtr& response)
{
    Api::ChunkReadRequest* req = static_cast<Api::ChunkReadRequest*>(request->GetData());
    if (request->GetDataSize() != sizeof(*req))
    {
        return response->Prepare(request->GetType(), Api::ResultUnexpectedDataSize, 0);
    }

    Api::ChunkReadResponse* resp = nullptr;
    Core::Error err = response->Prepare(request->GetType(), Api::ResultSuccess, sizeof(*resp));
    if (!err.Ok())
        return err;

    resp = static_cast<Api::ChunkReadResponse*>(response->GetData());
    err = ControlDevice::Get()->ChunkRead(req->ChunkId, resp->Data);
    if (!err.Ok())
    {
        err.Clear();
        response->SetResult(Api::ResultNotFound);
    }

    return err;
}

Core::Error Server::HandleChunkDelete(PacketPtr& request, PacketPtr& response)
{
    Core::Error err;

    Api::ChunkDeleteRequest* req = static_cast<Api::ChunkDeleteRequest*>(request->GetData());
    if (request->GetDataSize() != sizeof(*req))
    {
        return response->Prepare(request->GetType(), Api::ResultUnexpectedDataSize, 0);
    }

    err =  response->Prepare(request->GetType(), Api::ResultSuccess, 0);
    if (!err.Ok())
        return err;

    err = ControlDevice::Get()->ChunkDelete(req->ChunkId);
    if (!err.Ok())
    {
        err.Clear();
        response->SetResult(Api::ResultNotFound);
    }

    return response->Prepare(request->GetType(), Api::ResultSuccess, 0);
}

PacketPtr Server::HandleRequest(PacketPtr& request, Core::Error& err)
{
    PacketPtr response(new Packet());
    if (response.Get() == nullptr)
    {
        err = Core::Error::NoMemory;
        return response;
    }

    err = Core::Error::Success;
    switch (request->GetType())
    {
    case Api::PacketTypePing:
        err = HandlePing(request, response);
        break;
    case Api::PacketTypeChunkCreate:
        err = HandleChunkCreate(request, response);
        break;
    case Api::PacketTypeChunkWrite:
        err = HandleChunkWrite(request, response);
        break;
    case Api::PacketTypeChunkRead:
        err = HandleChunkRead(request, response);
        break;
    case Api::PacketTypeChunkDelete:
        err = HandleChunkDelete(request, response);
        break;
    default:
        err = response->Prepare(request->GetType(), Core::Error::UnknownCode, 0);
        break;
    }

    return response;
}

}