#pragma once
#include "Job.h"
#include "Tilemap.h"

struct PQNode
{
	PQNode(int32 cost, Vec2Int pos) : cost(cost), pos(pos) {}

	bool operator<(const PQNode& other) const { return cost < other.cost; }
	bool operator>(const PQNode& other) const { return cost > other.cost; }

	int32 cost;
	Vec2Int pos;
};

struct MyPQNode
{
	bool operator<(const MyPQNode& other)const { return f < other.f; }//less - ū�� �켱
	bool operator>(const MyPQNode& other)const { return f > other.f; }//greater - ������ �켱

	int f;
	int g;
	Vec2Int pos;

};

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
	void Update();
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
public:
	shared_ptr<Player> FindClosestPlayer(Vec2Int pos);
	bool FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool MyFindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool CanGo(Vec2Int cellPos);
	Vec2Int GetRandomEmptyCellPos();
	shared_ptr<GameObject> GetGameObjectAt(Vec2Int cellPos);
	shared_ptr<class Creature> GetCreatureAt(Vec2Int cellPos);
private:
	// ��Ŀ�����尡 ���ν����尡 ȣ���ϵ��� �ִ� Job
	JobQueue _jobs;
private:
	map<uint64, shared_ptr<class Player>> _players;
	map<uint64, shared_ptr<class Monster>> _monsters;

private:
	Tilemap _tilemap;
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

