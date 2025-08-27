#pragma once

#include "Protocol.pb.h"

constexpr int32 HANDLER_MAX = 0x2000;
using PacketHandlerFunc = std::function<bool(GameSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc g_packet_handler[HANDLER_MAX];

enum
{
PKT_S_EnterGame = 0,
PKT_S_MyPlayer = 1,
PKT_S_AddObject = 2,
PKT_S_RemoveObject = 3,
PKT_C_Move = 4,
PKT_S_Move = 5,
PKT_C_CHAT = 6,
PKT_S_CHAT = 7,
};

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length);
bool Handle_C_Move(GameSessionRef& session, Protocol::C_Move&pkt);
bool Handle_C_CHAT(GameSessionRef& session, Protocol::C_CHAT&pkt);

class ServerPacketHandler
{
public:
    static void Init()
    {
        for (int i = 0; i < HANDLER_MAX; ++i)
            g_packet_handler[i] = Handle_INVALID;
        g_packet_handler[PKT_C_Move] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::C_Move > (Handle_C_Move, session, buffer, length);
            };
        g_packet_handler[PKT_C_CHAT] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::C_CHAT > (Handle_C_CHAT, session, buffer, length);
            };
    }

    static bool HandlePacket(GameSessionRef session, BYTE * buffer, int32 length);
    static SendBufferRef MakeSendBuffer(const Protocol::S_EnterGame&pkt) { return MakeSendBuffer(pkt, PKT_S_EnterGame); }
    static SendBufferRef MakeSendBuffer(const Protocol::S_MyPlayer&pkt) { return MakeSendBuffer(pkt, PKT_S_MyPlayer); }
    static SendBufferRef MakeSendBuffer(const Protocol::S_AddObject&pkt) { return MakeSendBuffer(pkt, PKT_S_AddObject); }
    static SendBufferRef MakeSendBuffer(const Protocol::S_RemoveObject&pkt) { return MakeSendBuffer(pkt, PKT_S_RemoveObject); }
    static SendBufferRef MakeSendBuffer(const Protocol::S_Move&pkt) { return MakeSendBuffer(pkt, PKT_S_Move); }
    static SendBufferRef MakeSendBuffer(const Protocol::S_CHAT&pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT); }

    static SendBufferRef Make_S_EnterGame();
    static SendBufferRef Make_S_MyPlayer(const Protocol::ObjectInfo& info);
    static SendBufferRef Make_S_AddObject(const Protocol::S_AddObject& pkt);
    static SendBufferRef Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt);
    static SendBufferRef Make_S_Move(const Protocol::ObjectInfo& info);
private:
    template<typename PacketType, typename ProcessFunc>
    static bool ParsePacket(ProcessFunc func, GameSessionRef& session, BYTE * buffer, int32 length)
    {
        PacketType pkt;
        if (false == pkt.ParseFromArray(buffer + sizeof(PacketHeader), length - sizeof(PacketHeader)))
            return false;

        return func(session, pkt);
    }

    template<typename T>
    static SendBufferRef MakeSendBuffer(T& pkt, uint16 packet_id)
    {
        const uint16 data_size = static_cast<uint16>(pkt.ByteSizeLong());
        const uint16 packet_size = data_size + sizeof(PacketHeader);

        SendBufferRef send_buffer = make_shared<SendBuffer>(packet_size);

        PacketHeader* header = reinterpret_cast<PacketHeader*>(send_buffer->Buffer());
        header->size = packet_size;
        header->id = packet_id;

        assert(pkt.SerializeToArray(&header[1], data_size));
        send_buffer->Close(packet_size);

        return send_buffer;
    }
};