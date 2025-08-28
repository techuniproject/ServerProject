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
	void Init();//임시

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

