#include "pch.h"
#include "GameInstance.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "DevScene.h"
#include "MyPlayer.h"

void ClientPacketHandler::HandlePacket(ServerSessionRef session,BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	switch (header.id)
	{
	case S_TEST:
		Handle_S_TEST(session,buffer, len);
		break;
	case S_EnterGame:
		Handle_S_EnterGame(session,buffer, len);
		break;
	case S_MyPlayer:
		Handle_S_MyPlayer(session,buffer, len);
		break;
	case S_AddObject:
		Handle_S_AddObject(session,buffer, len);
		break;
	case S_RemoveObject:
		Handle_S_RemoveObject(session,buffer, len);
		break;
	}
}

void ClientPacketHandler::Handle_S_TEST(ServerSessionRef session,BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_TEST pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	uint64 id = pkt.id();
	uint32 hp = pkt.hp();
	uint16 attack = pkt.attack();

	cout << "ID: " << id << " HP : " << hp << " ATT : " << attack << endl;

	for (int32 i = 0; i < pkt.buffs_size(); i++)
	{
		const Protocol::BuffData& data = pkt.buffs(i);
		cout << "BuffInfo : " << data.buffid() << " " << data.remaintime() << endl;
	}
}

void ClientPacketHandler::Handle_S_EnterGame(ServerSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_EnterGame pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	bool success = pkt.success();
	uint64 accountId = pkt.accountid();
	
}

void ClientPacketHandler::Handle_S_MyPlayer(ServerSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_MyPlayer pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	const Protocol::ObjectInfo& info = pkt.info();

	GameInstance* gameInstance = GET_SINGLE(GameInstance);

	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
	if (scene)
	{
		shared_ptr<MyPlayer> myPlayer=scene->SpawnObject<MyPlayer>(Vec2Int(info.posx(), info.posy()));
		myPlayer->info = info;
		gameInstance->SetMyPlayer(myPlayer);
	}
}

void ClientPacketHandler::Handle_S_AddObject(ServerSessionRef session, BYTE* buffer, int32 len)
{
	//PacketHeader* header = (PacketHeader*)buffer;
	////uint16 id = header->id;
	//uint16 size = header->size;

	//Protocol::S_AddObject pkt;
	//pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	//bool success = pkt.success();
	//uint64 accountId = pkt.accountid();
}

void ClientPacketHandler::Handle_S_RemoveObject(ServerSessionRef session, BYTE* buffer, int32 len)
{
	//PacketHeader* header = (PacketHeader*)buffer;
	////uint16 id = header->id;
	//uint16 size = header->size;

	//Protocol::S_RemoveObject pkt;
	//pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	//bool success = pkt.success();
	//uint64 accountId = pkt.accountid();
}
