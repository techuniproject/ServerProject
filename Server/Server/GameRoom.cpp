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
	shared_ptr<GameRoom>gameRoom = shared_from_this();

	PushJob([gameRoom]() {
		shared_ptr<Monster> monster = GameObject::CreateMonster();
		monster->info.set_posx(8);
		monster->info.set_posy(8);
		gameRoom->Enter(monster);

		Protocol::S_AddObject AddedMonster;
		*AddedMonster.add_objects() = monster->info;

		SendBufferRef sendBuf= ServerPacketHandler::Make_S_AddObject(AddedMonster);
		gameRoom->Broadcast(sendBuf);
		});

//	shared_ptr<Monster> monster = GameObject::CreateMonster();
//	monster->info.set_posx(8);
//	monster->info.set_posy(8);
//	AddObject(monster);
}

void GameRoom::PushJob(function<void()> func)
{
	_jobs.Push(make_shared<LambdaJob>(move(func)));
}

void GameRoom::FlushJobs()
{
    while (true) {
        shared_ptr<Job> job = _jobs.Pop();
        if (!job) break;
        job->Execute();
    }
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

void GameRoom::Enter(shared_ptr<GameObject> gameObject)
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
	gameObject->room = shared_from_this();

}

void GameRoom::Leave(uint64 id)
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
}

void GameRoom::Broadcast(SendBufferRef sendBuffer)
{
    for (auto& p : _players)
        p.second->session->Send(sendBuffer);

}

vector<Protocol::ObjectInfo> GameRoom::GetRoomPlayerInfo()
{
	vector<Protocol::ObjectInfo> infos;

	for (auto p : _players)
		infos.push_back(p.second->info);

	return infos;
}

vector<Protocol::ObjectInfo> GameRoom::GetRoomMonsterInfo()
{
	vector<Protocol::ObjectInfo> infos;

	for (auto m : _monsters)
		infos.push_back(m.second->info);

	return infos;
}

//void GameRoom::Init()
//{
//	shared_ptr<Monster> monster = GameObject::CreateMonster();
//	monster->info.set_posx(8);
//	monster->info.set_posy(8);
//	AddObject(monster);
//}

//void GameRoom::Update()
//{
//
//}

//void GameRoom::EnterRoom(GameSessionRef session)
//{
//	shared_ptr<Player> curPlayer = GameObject::CreatePlayer();
//
//	//������ ���縦 ����
//	session->gameRoom = shared_from_this();
//	session->player = curPlayer;
//	curPlayer->session = session;
//
//	curPlayer->info.set_posx(5);
//	curPlayer->info.set_posy(5);
//	
//
//	//������ Ŭ�󿡰� ������ �����ֱ�
//	{
//		SendBufferRef sendBuffer=ServerPacketHandler::Make_S_MyPlayer(curPlayer->info);
//		session->Send(sendBuffer);
//	}
//	//��� ������Ʈ�� ���� ����
//	{
//		Protocol::S_AddObject pkt;
//
//		for (auto& item : _players)
//		{
//			//read - pkt.object(1) index
//			//write - Protocol::ObjectInfo* info =  pkt.add_objects() -pointer ��ȯ
//			Protocol::ObjectInfo* info = pkt.add_objects();
//			*info = item.second->info;
//		}
//
//		for (auto& item : _monsters)
//		{
//			Protocol::ObjectInfo* info = pkt.add_objects();
//			*info = item.second->info;
//		}
//
//		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
//		session->Send(sendBuffer);
//	}
//
//	AddObject(curPlayer);
//}
//
//void GameRoom::LeaveRoom(GameSessionRef session)
//{
//	if (session == nullptr)
//		return;
//	if (session->player.lock() == nullptr)
//		return;
//
//	uint64 id= session->player.lock()->info.objectid();
//	//shared_ptr<GameObject> gameObject = FindObject(id);
//	RemoveObject(id);
//}
//
//shared_ptr<GameObject> GameRoom::FindObject(uint64 id)
//{
//
//	{
//		auto findIt = _players.find(id);
//		if (findIt != _players.end())
//			return findIt->second;
//	}
//	{
//		auto findIt = _monsters.find(id);
//		if (findIt != _monsters.end())
//			return findIt->second;
//	}
//	return nullptr;
//}
//
//void GameRoom::Handle_C_Move(Protocol::C_Move& pkt)
//{
//	uint64 id = pkt.info().objectid();
//	shared_ptr<GameObject> gameObject= FindObject(id);
//	
//	if (gameObject == nullptr)
//		return;
//
//	//TODO Validation ��ŷ üŷ
//	gameObject->info.set_state(pkt.info().state());
//	gameObject->info.set_dir(pkt.info().dir());
//	gameObject->info.set_posx(pkt.info().posx());
//	gameObject->info.set_posy(pkt.info().posy());
//	
//	{
//		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(pkt.info());
//		Broadcast(sendBuffer);
//	}
//
//}
//
//void GameRoom::AddObject(shared_ptr<class GameObject> gameObject)
//{
//	uint64 id = gameObject->info.objectid();
//	auto objectType = gameObject->info.objecttype();
//	
//	switch (objectType)
//	{
//	case Protocol::OBJECT_TYPE_PLAYER:
//		_players[id] = static_pointer_cast<Player>(gameObject);
//		break;
//	case Protocol::OBJECT_TYPE_MONSTER:
//		_monsters[id] = static_pointer_cast<Monster>(gameObject);
//		break;
//	default:
//		return;
//	}
//	gameObject->room = GetRoomRef();
//
//	{
//		Protocol::S_AddObject pkt;
//
//		
//		Protocol::ObjectInfo* info = pkt.add_objects();
//		*info = gameObject->info; //���� �߰��� �� ����
//		
//		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
//		//session->Send(sendBuffer); ��� �ֵ����� ���������
//		Broadcast(sendBuffer);
//	}
//}
//
//void GameRoom::RemoveObject(uint64 id)
//{
//	shared_ptr<GameObject> gameObject = FindObject(id);
//	if (gameObject == nullptr)
//		return;
//
//	auto objectType = gameObject->info.objecttype();
//
//	switch (objectType)
//	{
//	case Protocol::OBJECT_TYPE_PLAYER:
//		_players.erase(id);
//		break;
//	case Protocol::OBJECT_TYPE_MONSTER:
//		_monsters.erase(id);
//		break;
//	default:
//		return;
//	}
//
//	gameObject->room = nullptr;
//
//	//TODO ������Ʈ ���� �޽��� ����
//
//	{
//		Protocol::S_RemoveObject pkt;
//		pkt.add_ids(id);
//		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
//		Broadcast(sendBuffer);
//	}
//}
//
//void GameRoom::Broadcast(SendBufferRef& sendBuffer)
//{
//	for (auto& item : _players)
//	{
//		item.second->session->Send(sendBuffer);
//	}
//}
//
