package main

import (
    "bytes"
    "encoding/binary"
    "errors"
    "fmt"
    "io"
    "log"
    "net"
    "os"
    "sync"
    "crypto/rand"
    "encoding/hex"
    "github.com/pborman/uuid"
    "github.com/OneOfOne/xxhash"
)

const (
    PacketMaxDataSize = 2 * 65536
    PacketMagic       = 0xCCBECCBE
    PacketTypePing    = 1
    PacketTypeChunkCreate = 2
    PacketTypeChunkWrite = 3
    PacketTypeChunkRead = 4
    PacketTypeChunkDelete = 5
    ChunkSize = 65536
    GuidSize = 16
    HashSize = 8
)

type Client struct {
    Host string
    Con  net.Conn
}

type PacketHeaderBase struct {
    Magic    uint32
    Type     uint32
    DataSize uint32
    Result   uint32
    DataHash [HashSize]byte
}

type PacketHeader struct {
    Base    PacketHeaderBase
    Hash    [HashSize]byte
}

type Packet struct {
    Header PacketHeader
    Body   []byte
}

type ToBytes interface {
    ToBytes() ([]byte, error)
}

type ParseBytes interface {
    ParseBytes(body []byte) error
}

type ReqPing struct {
    Value [PacketMaxDataSize]byte
}

type RespPing struct {
    Value [PacketMaxDataSize]byte
}

type ReqChunkCreate struct {
    ChunkId [GuidSize]byte
}

type RespChunkCreate struct {
}

type ReqChunkWrite struct {
    ChunkId [GuidSize]byte
    Data [ChunkSize]byte
}

type RespChunkWrite struct {
}

type ReqChunkRead struct {
    ChunkId [GuidSize]byte
}

type RespChunkRead struct {
    Data [ChunkSize]byte
}

type ReqChunkDelete struct {
    ChunkId [GuidSize]byte
}

type RespChunkDelete struct {
}

func (req *ReqPing) ToBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, req)
    if err != nil {
        return nil, err
    }

    return buf.Bytes(), nil
}

func (resp *RespPing) ParseBytes(body []byte) error {
    err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
    if err != nil {
        return err
    }
    return nil
}

func (req *ReqChunkCreate) ToBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, req)
    if err != nil {
        return nil, err
    }
    return buf.Bytes(), nil
}

func (resp *RespChunkCreate) ParseBytes(body []byte) error {
    err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
    if err != nil {
        return err
    }
    return nil
}

func (req *ReqChunkWrite) ToBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, req)
    if err != nil {
        return nil, err
    }
    return buf.Bytes(), nil
}

func (resp *RespChunkWrite) ParseBytes(body []byte) error {
    err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
    if err != nil {
        return err
    }
    return nil
}

func (req *ReqChunkRead) ToBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, req)
    if err != nil {
        return nil, err
    }
    return buf.Bytes(), nil
}

func (resp *RespChunkRead) ParseBytes(body []byte) error {
    err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
    if err != nil {
        return err
    }
    return nil
}

func (req *ReqChunkDelete) ToBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, req)
    if err != nil {
        return nil, err
    }

    return buf.Bytes(), nil
}

func (resp *RespChunkDelete) ParseBytes(body []byte) error {
    err := binary.Read(bytes.NewReader(body), binary.LittleEndian, resp)
    if err != nil {
        return err
    }
    return nil
}

func NewClient(host string) *Client {
    client := new(Client)
    client.Host = host
    return client
}

func (client *Client) Dial() error {
    con, err := net.Dial("tcp", client.Host)
    if err != nil {
        return err
    }
    client.Con = con
    return nil
}

func (header *PacketHeaderBase) GetBytes() ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, header)
    if err != nil {
        return nil, err
    }

    return buf.Bytes(), nil
}

func getUint64Bytes(val uint64) ([]byte, error) {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, &val)
    if err != nil {
        return nil, err
    }

    return buf.Bytes(), nil
}

func getHash(data []byte) ([]byte, error) {
    h := xxhash.New64()
    h.Write(data)
    return getUint64Bytes(h.Sum64())
}

func (client *Client) CreatePacket(packetType uint32, body []byte) (*Packet, error) {
    packet := new(Packet)
    packet.Header.Base.Magic = PacketMagic
    packet.Header.Base.Type = packetType
    packet.Header.Base.DataSize = uint32(len(body))
    packet.Header.Base.Result = 0

    hash, err := getHash(body)
    if err != nil {
        return nil, err
    }

    copy(packet.Header.Base.DataHash[:], hash)

    buf, err := packet.Header.Base.GetBytes()
    if err != nil {
        return nil, err
    }

    hash, err = getHash(buf)
    if err != nil {
        return nil, err
    }

    copy(packet.Header.Hash[:], hash)

    packet.Body = body
    return packet, nil
}

func (client *Client) SendPacket(packet *Packet) error {
    buf := new(bytes.Buffer)
    err := binary.Write(buf, binary.LittleEndian, &packet.Header)
    if err != nil {
        return err
    }

    err = binary.Write(buf, binary.LittleEndian, packet.Body)
    if err != nil {
        return err
    }

    n, err := client.Con.Write(buf.Bytes())
    if err != nil {
        return err
    }

    if n != buf.Len() {
        return errors.New("Incomplete I/O")
    }

    return nil
}

func (client *Client) RecvPacket() (*Packet, error) {
    packet := new(Packet)
    err := binary.Read(client.Con, binary.LittleEndian, &packet.Header)
    if err != nil {
        return nil, err
    }

    if packet.Header.Base.Magic != PacketMagic {
        return nil, errors.New("Invalid packet magic")
    }

    if packet.Header.Base.DataSize > PacketMaxDataSize {
        return nil, errors.New("Packet data size too big")
    }

    buf, err := packet.Header.Base.GetBytes()
    if err != nil {
        return nil, err
    }

    hash, err := getHash(buf)
    if err != nil {
        return nil, err
    }


    if !bytes.Equal(hash, packet.Header.Hash[:]) {
        return nil, errors.New("Packet header data corruption")
    }


    body := make([]byte, packet.Header.Base.DataSize)
    if packet.Header.Base.DataSize != 0 {
        n, err := io.ReadFull(client.Con, body)
        if err != nil {
            return nil, err
        }

        if uint32(n) != packet.Header.Base.DataSize {
            return nil, errors.New("Incomplete I/O")
        }

        hash, err := getHash(body)
        if err != nil {
            return nil, err
        }

        if !bytes.Equal(hash, packet.Header.Base.DataHash[:]) {
            return nil, errors.New("Packet data corruption")
        }
    }
    packet.Body = body
    return packet, nil
}

func (client *Client) MakePacket(reqType uint32, req ToBytes) (*Packet, error) {
    body, err := req.ToBytes()
    if err != nil {
        return nil, err
    }
    return client.CreatePacket(reqType, body)
}

func (client *Client) SendRequest(reqType uint32, req ToBytes) error {
    packet, err := client.MakePacket(reqType, req)
    if err != nil {
        return err
    }

    return client.SendPacket(packet)
}

func (client *Client) RecvResponse(respType uint32, resp ParseBytes) error {
    packet, err := client.RecvPacket()
    if err != nil {
        return err
    }

    if packet.Header.Base.Type != respType {
        return fmt.Errorf("Unexpected packet type %d, should be %d",
			packet.Header.Base.Type, respType)
    }

    if packet.Header.Base.Result != 0 {
        return fmt.Errorf("Packet error: %d", int32(packet.Header.Base.Result))
    }

    return resp.ParseBytes(packet.Body)
}

func (client *Client) SendRecv(reqType uint32, req ToBytes, resp ParseBytes) error {
    err := client.SendRequest(reqType, req)
    if err != nil {
        return err
    }
    err = client.RecvResponse(reqType, resp)
    if err != nil {
        return err
    }

    return nil
}

func getString(bytes []byte) string {
    for i, _ := range bytes {
        if bytes[i] == 0 {
            return string(bytes[:i])
        }
    }

    return string(bytes[:len(bytes)])
}

func (client *Client) Ping(value string) (string, error) {
    req := new(ReqPing)
    valueBytes := []byte(value)
    if len(valueBytes) > len(req.Value) {
        return "", errors.New("Key too big")
    }
    copy(req.Value[:len(req.Value)], valueBytes)

    resp := new(RespPing)
    err := client.SendRecv(PacketTypePing, req, resp)
    if err != nil {
        return "", err
    }

    return getString(resp.Value[:len(resp.Value)]), nil
}

func (client *Client) ChunkCreate(chunkId []byte) error {
    req := new(ReqChunkCreate)
    if len(chunkId) != len(req.ChunkId) {
        return errors.New("Invalid chunk id size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])

    resp := new(RespChunkCreate)
    err := client.SendRecv(PacketTypeChunkCreate, req, resp)
    if err != nil {
        return err
    }

    return nil
}

func (client *Client) ChunkWrite(chunkId []byte, data []byte) (error) {
    req := new(ReqChunkWrite)
    if len(chunkId) != len(req.ChunkId) {
        return errors.New("Invalid chunk id size")
    }
    if len(data) != len(req.Data) {
        return errors.New("Invalid data size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])
    copy(req.Data[:len(req.Data)], data[:len(req.Data)])

    resp := new(RespChunkWrite)
    err := client.SendRecv(PacketTypeChunkWrite, req, resp)
    if err != nil {
        return err
    }

    return nil
}

func (client *Client) ChunkRead(chunkId []byte) ([]byte, error) {
    req := new(ReqChunkRead)
    if len(chunkId) != len(req.ChunkId) {
        return nil, errors.New("Invalid chunk id size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])

    resp := new(RespChunkRead)
    err := client.SendRecv(PacketTypeChunkRead, req, resp)
    if err != nil {
        return nil, err
    }

    return resp.Data[:len(resp.Data)], nil
}

func (client *Client) ChunkDelete(chunkId []byte) error {
    req := new(ReqChunkDelete)
    if len(chunkId) != len(req.ChunkId) {
        return errors.New("Invalid chunk id size")
    }
    copy(req.ChunkId[:len(req.ChunkId)], chunkId[:len(req.ChunkId)])

    resp := new(RespChunkDelete)
    err := client.SendRecv(PacketTypeChunkDelete, req, resp)
    if err != nil {
        return err
    }

    return nil
}

func (client *Client) Close() {
    if client.Con != nil {
        client.Con.Close()
    }
}

func testClient(rounds int, client *Client, wg *sync.WaitGroup) error {
    defer wg.Done()

    for i := 0; i < rounds; i++ {
        chunkId := uuid.NewRandom()[:]
        chunkIdS := hex.EncodeToString(chunkId)

        err := client.ChunkCreate(chunkId)
        if err != nil {
            log.Printf("Chunk %s create failed: %v\n", chunkIdS, err)
            return err
        }

        data := make([]byte, ChunkSize)
        _, err = rand.Read(data)
        if err != nil {
            log.Printf("Chunk %s rand fill failed: %v\n", chunkIdS, err)
            return err
        }

        err = client.ChunkWrite(chunkId, data)
        if err != nil {
            log.Printf("Chunk %s write failed: %v\n", chunkIdS, err)
            return err
        }

        dataRead, err := client.ChunkRead(chunkId)
        if err != nil {
            log.Printf("Chunk %s read failed: %v\n", chunkIdS, err)
            return err
        }

        if !bytes.Equal(data, dataRead) {
            err = errors.New("Unexpected data read")
            log.Printf("Chunk %s read failed: %v\n", chunkIdS, err)
            return err
        }

        err = client.ChunkDelete(chunkId)
        if err != nil {
            log.Printf("Chunk %s delete failed: %v\n", chunkIdS, err)
            return err
        }
    }
    return nil
}

func main() {
    log.SetFlags(0)
    log.SetOutput(os.Stdout)

//  log.Printf("Open clients\n")
    clients := make([]*Client, 0)
    for i := 0; i < 1000; i++ {
        client := NewClient("127.0.0.1:8111")
        err := client.Dial()
        if err != nil {
            log.Printf("Dial failed: %v\n", err)
            os.Exit(1)
	    return
        }
        clients = append(clients, client)
    }

//  log.Printf("Exchange information\n")
    wg := new(sync.WaitGroup)
    for _, client := range clients {
        wg.Add(1)
        go testClient(1, client, wg)
    }
    wg.Wait()

//  log.Printf("Close clients\n")
    for _, client := range clients {
        client.Close()
    }
}
