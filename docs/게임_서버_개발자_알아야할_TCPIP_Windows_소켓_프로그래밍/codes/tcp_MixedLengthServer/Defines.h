#pragma once


#include <cstdint>

// ��Ŷ ���� ����
enum PacketType {
    PT_TRADE_REQUEST = 1,
    PT_TRADE_RESPONSE,
    PT_CHAT_MESSAGE,
    PT_PLAYER_MOVE
};

// ��Ŷ ��� (���� ����)
#pragma pack(push, 1)
struct PacketHeader {
    uint16_t totalSize;     // ��� ���� ��ü ��Ŷ ũ��
    uint16_t packetType;    // ��Ŷ ���� (PacketType enum)
};

// ������ ����ü (���� ����)
struct Item {
    uint32_t itemId;        // ������ ���� ID
    uint16_t quantity;      // ����
    uint16_t category;      // ī�װ� (����, �� ��)
};

// �ŷ� ��û ��Ŷ (���� ���� + ���� ����)
struct TradeRequestPacket {
    uint32_t requesterId;   // ��û�� ID
    uint32_t targetId;      // ����� ID
    uint16_t itemCount;     // ������ ��
    // ���Ŀ� Item �迭�� �߰� �޽���(���ڿ�)�� ���� ���̷� ��
};
#pragma pack(pop)