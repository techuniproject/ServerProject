#pragma once

#include "Protocol.pb.h"

constexpr int32 HANDLER_MAX = 0x2000;
using PacketHandlerFunc = std::function<bool(GameSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc g_packet_handler[HANDLER_MAX];

enum
{
    {%- for packet in parser.total_packet %}
PKT_{{packet.name}} = {{packet.id}},
{%- endfor %}
};

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length);
{%- for packet in parser.recv_packet %}
bool Handle_{{packet.name}}(GameSessionRef& session, Protocol::{{packet.name}}&pkt);
{%- endfor %}

class {{ output }}
{
public:
    static void Init()
    {
        for (int i = 0; i < HANDLER_MAX; ++i)
            g_packet_handler[i] = Handle_INVALID;
        {%- for packet in parser.recv_packet %}
        g_packet_handler[PKT_{{packet.name}}] = [](GameSessionRef& session, BYTE* buffer, int32 length)
            {
                return ParsePacket < Protocol::{{packet.name}} > (Handle_{{packet.name}}, session, buffer, length);
            };
        {%- endfor %}
    }

    static bool HandlePacket(GameSessionRef& session, BYTE * buffer, int32 length);
    {%- for packet in parser.send_packet %}
    static SendBufferRef MakeSendBuffer(Protocol::{{packet.name}}&pkt) { return MakeSendBuffer(pkt, PKT_{{packet.name}}); }
    {%- endfor %}

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
