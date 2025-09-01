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
	void Init();//임시
	void Update();
	shared_ptr<class GameObject> FindObject(uint64 id);
	void PushJob(function<void()> func);
	void FlushJobs();
	void Enter(shared_ptr<GameObject> gameObject);
	void Leave(uint64 id);
	void Broadcast(SendBufferRef sendBuffer);
	
	// 현재 게임룸에서 관리하는 게임오브젝트 정보 전파용도
	vector<Protocol::ObjectInfo> GetRoomPlayerInfo();
	vector<Protocol::ObjectInfo> GetRoomMonsterInfo();
	// 위 두 함수가 메인 스레드가 작업 큐에서 꺼내서 사용할땐 문제없음
	//하지만, 워커스레드가(GameSession, SeverPacketHandler)에서 입출력이와서 
	// 이 함수를 사용한다면, 컨테이너에 대한 정보가 복사본으로 넘어가기때문에 race condition
	// 은 피할 수 있지만, 그 상황에서 메인스레드가 작업처리중이면 데이터 불일치 생길 수 있음
	// 그러므로 워커스레드가 위 두함수를 사용시 데이터 불일치 감안 또는 메인이 다 처리하도록 전달.

	map<uint64, shared_ptr<class Player>>& GetPlayersForJob() { return _players; }
	map<uint64, shared_ptr<class Monster>>& GetMonstersForJob() { return _monsters; }
public:
	shared_ptr<Player> FindClosestPlayer(Vec2Int pos);
	bool FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool CanGo(Vec2Int cellPos);
	Vec2Int GetRandomEmptyCellPos();
	shared_ptr<GameObject> GetGameObjectAt(Vec2Int cellPos);
	shared_ptr<class Creature> GetCreatureAt(Vec2Int cellPos);
public:
 // 메인 스레드에서 I/O 예약
    void PushSendJob(shared_ptr<class GameSession> session, SendBufferRef sendBuf);
    void PushBroadcastJob(SendBufferRef sendBuf);

    // 워커 스레드에서 Flush 실행
    void FlushSendJobs();
    void FlushBroadcastJobs();
private:
	// I/O 예약 처리(메인스레드가 호출)
	mutex _sendLock;
	mutex _broadcastLock; //병렬적으로 send, broadcast 다른 워커스레드간 처리가능하도록 lock분리

	queue<pair<weak_ptr<class GameSession>, SendBufferRef>> _sendJobs;
	queue<SendBufferRef> _broadcastJobs;
private:
	// 워커스레드가 메인스레드가 호출하도록 넣는 Job
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

