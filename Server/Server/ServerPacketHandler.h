#pragma once

enum
{
	S_TEST = 1,
	S_EnterGame = 2, //���п뵵

	S_MyPlayer = 4, //���п뵵
	S_AddObject = 5, //���п뵵
	S_RemoveObject = 6,//���п뵵

	C_Move = 10,
	S_Move = 11,
};

struct BuffData
{
	uint64 buffId;
	float remainTime;
};

class ServerPacketHandler
{
public:
	static void HandlePacket(GameSessionRef session, BYTE* buffer, int32 len);

	// �ޱ�
	static void Handle_C_Move(GameSessionRef session, BYTE* buffer, int32 len);

	// ������
	static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs);
	static SendBufferRef Make_S_EnterGame();
	static SendBufferRef Make_S_MyPlayer(const Protocol::ObjectInfo& info);
	static SendBufferRef Make_S_AddObject(const Protocol::S_AddObject& pkt);
	static SendBufferRef Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt);
	static SendBufferRef Make_S_Move(const Protocol::ObjectInfo& info);

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		assert(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};

// https://minusi.tistory.com/entry/%EA%B5%AC%EA%B8%80-%ED%94%84%EB%A1%9C%ED%86%A0%EC%BD%9C-%EB%B2%84%ED%8D%BC-%EC%82%AC%EC%9A%A9%ED%95%98%EA%B8%B0with-visual-studio