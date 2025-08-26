#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"

PacketHandlerFunc g_packet_handler[HANDLER_MAX];

bool ServerPacketHandler::HandlePacket(GameSessionRef session, BYTE* buffer, int32 length)
{
    // 1) 헤더 읽기
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // 2) id 범위 체크
    if (header->id >= HANDLER_MAX)
        return false;

    // 3) 등록된 핸들러 실행
    return g_packet_handler[header->id](session, buffer, length);
}

SendBufferRef ServerPacketHandler::Make_S_EnterGame()
{
    Protocol::S_EnterGame packet;
 
 	packet.set_success(true);
 	packet.set_accountid(0);
 
 	return MakeSendBuffer(packet);//Serialize
}

SendBufferRef ServerPacketHandler::Make_S_MyPlayer(const Protocol::ObjectInfo& info)
{
    Protocol::S_MyPlayer pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info(); //message구성하는 struct pointer반환

	*objectInfo = info;

	return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_AddObject(const Protocol::S_AddObject& pkt)
{
	return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt)
{
    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Move(const Protocol::ObjectInfo& info)
{
    Protocol::S_Move pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info();
	*objectInfo = info;

    return MakeSendBuffer(pkt);
}

////서버에 보낸 클라(세션)을 알기위해 받는 첫 인자
//void ServerPacketHandler::HandlePacket(GameSessionRef session, BYTE* buffer, int32 len)
//{
//	BufferReader br(buffer, len);
//
//	PacketHeader header;
//	br.Peek(&header);
//
//	switch (header.id)
//	{
//	case C_Move:
//		Handle_C_Move(session, buffer, len);
//		break;
//	default:
//		break;
//	}
//}

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length)
{
    //초기 함수 포인터 설정용 함수
    return false;
}

bool Handle_C_Move(GameSessionRef& session, Protocol::C_Move& pkt)//받아와서 시리얼라이즈해줌
{
   /* if (GRoom) 
    {
        GRoom->Handle_C_Move(pkt);
        return true;
    }
 
    return false;*/

    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->Handle_C_Move(pkt);
        return true;
    }
    return false;
}


//void ServerPacketHandler::Handle_C_Move(GameSessionRef session, BYTE* buffer, int32 len)
//{
//
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::C_Move pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//
//	shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
//	if (gameRoom)
//		gameRoom->Handle_C_Move(pkt);
//}


//SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs)
//{
//	Protocol::S_TEST pkt;
//
//	pkt.set_id(10);
//	pkt.set_hp(100);
//	pkt.set_attack(10);
//
//	{
//		Protocol::BuffData* data = pkt.add_buffs();
//		data->set_buffid(100);
//		data->set_remaintime(1.2f);
//		{
//			data->add_victims(10);
//		}
//	}
//	{
//		Protocol::BuffData* data = pkt.add_buffs();
//		data->set_buffid(200);
//		data->set_remaintime(2.2f);
//		{
//			data->add_victims(20);
//		}
//	}
//
//	return MakeSendBuffer(pkt, S_TEST);
//}

////[2][2][    ] 
//SendBufferRef ServerPacketHandler::Make_S_EnterGame()
//{
//	Protocol::S_EnterGame packet;
//
//	packet.set_success(true);
//	packet.set_accountid(0);
//
//	return MakeSendBuffer(packet,S_EnterGame);//Serialize
//}
//
//SendBufferRef ServerPacketHandler::Make_S_MyPlayer(const Protocol::ObjectInfo& info)
//{
//	Protocol::S_MyPlayer pkt;
//
//	Protocol::ObjectInfo* objectInfo = pkt.mutable_info(); //message구성하는 struct pointer반환
//
//	*objectInfo = info;
//
//	return MakeSendBuffer(pkt, S_MyPlayer);
//}
//
//SendBufferRef ServerPacketHandler::Make_S_AddObject(const Protocol::S_AddObject& pkt)
//{
//	//Protocol::S_AddObject pkt;
//
//	//Protocol::ObjectInfo* objectInfo = pkt.mutable_info(); //message구성하는 struct pointer반환
//
//	//*objectInfo = info;
//
//	//return MakeSendBuffer(pkt, S_AddObject);
//	return MakeSendBuffer(pkt,S_AddObject);
//}
//
//SendBufferRef ServerPacketHandler::Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt)
//{
//	return MakeSendBuffer(pkt,S_RemoveObject);
//}
//
//SendBufferRef ServerPacketHandler::Make_S_Move(const Protocol::ObjectInfo& info)
//{
//	Protocol::S_Move pkt;
//
//	Protocol::ObjectInfo* objectInfo = pkt.mutable_info();
//	*objectInfo = info;
//
//	return MakeSendBuffer(pkt,S_Move);
//}


