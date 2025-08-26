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

class ClientPacketHandler
{
public:
	static void HandlePacket(ServerSessionRef session, BYTE* buffer, int32 len);

	// �ޱ�
	static void Handle_S_TEST(ServerSessionRef session, BYTE* buffer, int32 len);
	static void Handle_S_EnterGame(ServerSessionRef session, BYTE* buffer, int32 len);
	static void Handle_S_MyPlayer(ServerSessionRef session, BYTE* buffer, int32 len);
	static void Handle_S_AddObject(ServerSessionRef session, BYTE* buffer, int32 len);
	static void Handle_S_RemoveObject(ServerSessionRef session, BYTE* buffer, int32 len);
	static void Handle_S_Move(ServerSessionRef session, BYTE* buffer, int32 len);


	// ������
	static SendBufferRef Make_C_Move();


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

