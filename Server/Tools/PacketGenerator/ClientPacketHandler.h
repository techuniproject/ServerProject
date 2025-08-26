#pragma once

#include "Protocol.pb.h"

constexpr int32 HANDLER_MAX = 0x2000;
using PacketHandlerFunc = std::function<bool(GameSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc g_packet_handler[HANDLER_MAX];

enum
{
PKT_S_TEST = 0,
PKT_S_EnterGame = 1,
PKT_S_MyPlayer = 2,
PKT_S_AddObject = 3,
PKT_S_RemoveObject = 4,
PKT_C_Move = 5,
PKT_S_Move = 6,
};

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length);
bool Handle_S_TEST(GameSessionRef& session, Protocol::S_TEST&pkt);
bool Handle_S_EnterGame(GameSessionRef& session, Protocol::S_EnterGame&pkt);
bool Handle_S_MyPlayer(GameSessionRef& session, Protocol::S_MyPlayer&pkt);
bool Handle_S_AddObject(GameSessionRef& session, Protocol::S_AddObject&pkt);
bool Handle_S_RemoveObject(GameSessionRef& session, Protocol::S_RemoveObject&pkt);
bool Handle_S_Move(GameSessionRef& session, Protocol::S_Move&pkt);

class ClientPacketHandler
{
public:
    static void Init()
    {
        for (int i = 0; i < HANDLER_MAX; ++i)
            g_packet_handler[i] = Handle_INVALID;
        g_packet_handler[PKT_S_TEST] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_TEST > (Handle_S_TEST, session, buffer, length);
            };
        g_packet_handler[PKT_S_EnterGame] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_EnterGame > (Handle_S_EnterGame, session, buffer, length);
            };
        g_packet_handler[PKT_S_MyPlayer] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_MyPlayer > (Handle_S_MyPlayer, session, buffer, length);
            };
        g_packet_handler[PKT_S_AddObject] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_AddObject > (Handle_S_AddObject, session, buffer, length);
            };
        g_packet_handler[PKT_S_RemoveObject] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_RemoveObject > (Handle_S_RemoveObject, session, buffer, length);
            };
        g_packet_handler[PKT_S_Move] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::S_Move > (Handle_S_Move, session, buffer, length);
            };
    }

    static bool HandlePacket(GameSessionRef& session, BYTE * buffer, int32 length);
    static SendBufferRef MakeSendBuffer(Protocol::C_Move&pkt) { return MakeSendBuffer(pkt, PKT_C_Move); }

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