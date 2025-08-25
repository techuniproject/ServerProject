#include "pch.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"

shared_ptr<GameRoom> GRoom = make_shared<GameRoom>();

GameRoom::GameRoom()
{

}

GameRoom::~GameRoom()
{
}

void GameRoom::Init()
{
	shared_ptr<Monster> monster = GameObject::CreateMonster();
	monster->info.set_posx(8);
	monster->info.set_posy(8);
	AddObject(monster);
}

void GameRoom::Update()
{

}

void GameRoom::EnterRoom(GameSessionRef session)
{
	shared_ptr<Player> curPlayer = GameObject::CreatePlayer();

	//서로의 존재를 연결
	session->gameRoom = GetRoomRef();
	session->player = curPlayer;
	curPlayer->session = session;

	curPlayer->info.set_posx(5);
	curPlayer->info.set_posy(5);

	//입장한 클라에게 정보를 보내주기
	{
		SendBufferRef sendBuffer=ServerPacketHandler::Make_S_MyPlayer(curPlayer->info);
		session->Send(sendBuffer);
	}
	//모든 오브젝트에게 정보 전송
	{
		Protocol::S_AddObject pkt;

		for (auto& item : _players)
		{
			//read - pkt.object(1) index
			//write - Protocol::ObjectInfo* info =  pkt.add_objects() -pointer 반환
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		for (auto& item : _monsters)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		session->Send(sendBuffer);
	}

	AddObject(curPlayer);
}

void GameRoom::LeaveRoom(GameSessionRef session)
{
	if (session == nullptr)
		return;
	if (session->player.lock() == nullptr)
		return;

	uint64 id= session->player.lock()->info.objectid();
	//shared_ptr<GameObject> gameObject = FindObject(id);
	RemoveObject(id);
}

shared_ptr<GameObject> GameRoom::FindObject(uint64 id)
{

	{
		auto findIt = _players.find(id);
		if (findIt != _players.end())
			return findIt->second;
	}
	{
		auto findIt = _monsters.find(id);
		if (findIt != _monsters.end())
			return findIt->second;
	}
	return nullptr;
}

void GameRoom::AddObject(shared_ptr<class GameObject> gameObject)
{
	uint64 id = gameObject->info.objectid();
	auto objectType = gameObject->info.objecttype();
	
	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players[id] = static_pointer_cast<Player>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters[id] = static_pointer_cast<Monster>(gameObject);
		break;
	default:
		return;
	}
	gameObject->room = GetRoomRef();

	{
		Protocol::S_AddObject pkt;

		
		Protocol::ObjectInfo* info = pkt.add_objects();
		*info = gameObject->info; //현재 추가될 애 정보
		
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		//session->Send(sendBuffer); 모든 애들한테 보내줘야함
		Broadcast(sendBuffer);
	}
}

void GameRoom::RemoveObject(uint64 id)
{
	shared_ptr<GameObject> gameObject = FindObject(id);
	if (gameObject == nullptr)
		return;

	auto objectType = gameObject->info.objecttype();

	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players.erase(id);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters.erase(id);
		break;
	default:
		return;
	}

	gameObject->room = nullptr;

	//TODO 오브젝트 삭제 메시지 전송

	{
		Protocol::S_RemoveObject pkt;
		pkt.add_ids(id);
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::Broadcast(SendBufferRef& sendBuffer)
{

}
