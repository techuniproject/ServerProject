#pragma once
#include "Job.h"

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

//	void Init();
//	void Update();
//
//	void EnterRoom(GameSessionRef session);
//	void LeaveRoom(GameSessionRef session);
//	shared_ptr<class GameObject> FindObject(uint64 id);
//
//	shared_ptr<GameRoom> GetRoomRef() { return shared_from_this(); }
//public:
//	//PacketHandler
//	void Handle_C_Move(Protocol::C_Move& pkt);
//public:
//	void AddObject(shared_ptr<class GameObject> gameObject);
//	void RemoveObject(uint64 id);
//	void Broadcast(SendBufferRef& sendBuffer);
public:
	void Init();//�ӽ�

	shared_ptr<class GameObject> FindObject(uint64 id);
	void PushJob(function<void()> func);
	void FlushJobs();
	void Enter(shared_ptr<GameObject> gameObject);
	void Leave(uint64 id);
	void Broadcast(SendBufferRef sendBuffer);
	
	// ���� ���ӷ뿡�� �����ϴ� ���ӿ�����Ʈ ���� ���Ŀ뵵
	vector<Protocol::ObjectInfo> GetRoomPlayerInfo();
	vector<Protocol::ObjectInfo> GetRoomMonsterInfo();
	// �� �� �Լ��� ���� �����尡 �۾� ť���� ������ ����Ҷ� ��������
	//������, ��Ŀ�����尡(GameSession, SeverPacketHandler)���� ������̿ͼ� 
	// �� �Լ��� ����Ѵٸ�, �����̳ʿ� ���� ������ ���纻���� �Ѿ�⶧���� race condition
	// �� ���� �� ������, �� ��Ȳ���� ���ν����尡 �۾�ó�����̸� ������ ����ġ ���� �� ����
	// �׷��Ƿ� ��Ŀ�����尡 �� ���Լ��� ���� ������ ����ġ ���� �Ǵ� ������ �� ó���ϵ��� ����.
	
	map<uint64, shared_ptr<class Player>>& GetPlayersForJob() { return _players; }
	map<uint64, shared_ptr<class Monster>>& GetMonstersForJob() { return _monsters; }

private:
	map<uint64, shared_ptr<class Player>> _players;
	map<uint64, shared_ptr<class Monster>> _monsters;
	JobQueue _jobs;
};

extern shared_ptr<GameRoom> GRoom;


//class GameRoom : public enable_shared_from_this<GameRoom>
//{
//public:
//	GameRoom();
//	virtual ~GameRoom();
//
//	void Init();
//	void Update();
//
//	void EnterRoom(GameSessionRef session);
//	void LeaveRoom(GameSessionRef session);
//	shared_ptr<class GameObject> FindObject(uint64 id);
//
//	shared_ptr<GameRoom> GetRoomRef() { return shared_from_this(); }
//public:
//	//PacketHandler
//	void Handle_C_Move(Protocol::C_Move& pkt);
//public:
//	void AddObject(shared_ptr<class GameObject> gameObject);
//	void RemoveObject(uint64 id);
//	void Broadcast(SendBufferRef& sendBuffer);
//
//private:
//	map<uint64, shared_ptr<class Player>> _players;
//	map<uint64, shared_ptr<class Monster>> _monsters;
//
//};

