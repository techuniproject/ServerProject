#pragma once
#include "Job.h"
#include "Item.h"
#include "Tilemap.h"
#include "Sector.h"

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
	bool operator<(const MyPQNode& other)const { return f < other.f; }//less - 큰게 우선
	bool operator>(const MyPQNode& other)const { return f > other.f; }//greater - 작은게 우선

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
	void Init();//임시
	void Update();
	shared_ptr<class GameObject> FindObject(uint64 id);
	void PushJob(function<void()> func);
	void FlushJobs();
	void Enter(shared_ptr<GameObject> gameObject);
	void Leave(uint64 id);
	void Broadcast(SendBufferRef sendBuffer);
	void BroadcastBySector(SendBufferRef sendBuffer, Vec2Int SectorPos);
	void AddItem(Item item);
	void DeleteItem(uint32 id);
	
	// 현재 게임룸에서 관리하는 게임오브젝트 정보 전파용도
	vector<Protocol::ObjectInfo> GetRoomPlayerInfo();
	vector<Protocol::ObjectInfo> GetRoomMonsterInfo();
	vector<Protocol::ObjectInfo> GetRoomArrowInfo();
	vector<Protocol::ItemInfo> GetRoomItemInfo();
	// 위 두 함수가 메인 스레드가 작업 큐에서 꺼내서 사용할땐 문제없음
	//하지만, 워커스레드가(GameSession, SeverPacketHandler)에서 입출력이와서 
	// 이 함수를 사용한다면, 컨테이너에 대한 정보가 복사본으로 넘어가기때문에 race condition
	// 은 피할 수 있지만, 그 상황에서 메인스레드가 작업처리중이면 데이터 불일치 생길 수 있음
	// 그러므로 워커스레드가 위 두함수를 사용시 데이터 불일치 감안 또는 메인이 다 처리하도록 전달.

	map<uint64, shared_ptr<class Player>>& GetPlayersForJob() { return _players; }
	map<uint64, shared_ptr<class Monster>>& GetMonstersForJob() { return _monsters; }
	map<uint64, shared_ptr<class Arrow>>& GetArrowsForJob() { return _arrows; }
	map<uint32, Item>& GetItemsForJob() { return _items; }
public:
	shared_ptr<Player> FindClosestPlayer(Vec2Int pos);
	bool FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool MyFindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool CanGo(Vec2Int cellPos);
	Vec2Int GetRandomEmptyCellPos();
	shared_ptr<GameObject> GetGameObjectAt(Vec2Int cellPos);
	shared_ptr<class Creature> GetCreatureAt(Vec2Int cellPos);
	optional<Item> GetItemAt(Vec2Int cellPos);
	void AddDeleteProjectiletoList(uint64 idx);
	void TickMonsterSpawn();
	void DeleteProjectiles();
public:
	//Sector관련 모든 서버에서관리하는 Creature들은 생성 시에 SectorPos갱신 및 컨테이너에 추가
	Sector* GetSectorAt(Vec2Int sectorPos);
	Vec2Int GetSectorPos(int32 x, int32 y);
	void InsertAtSector(Vec2Int sectorPos, GameObject* creature);
	void DeleteFromSector(GameObject* creature);
	bool CheckValidSectorPos(Vec2Int sectorPos);


	bool CanGoBySector(Vec2Int cellPos);
	//목적 : 정확히 그 위치에 있는지 
	Player* GetPlayerAtSector(Vec2Int cellPos);
	Monster* GetMonsterAtSector(Vec2Int cellPos);
	Arrow* GetArrowAtSector(Vec2Int cellPos);
	Creature* GetCreatureAtSector(Vec2Int cellPos);
	weak_ptr<Player> FindClosestPlayerBySector(Vec2Int cellPos);
	//댕글링포인터 때문에 rawpointer반환 x

	void DoSomethingCrossingSectors(Vec2Int curCellPos, Vec2Int LastCellPos, function<void(Vec2Int)>add, function<void(Vec2Int)>remove);
	
private:
	// 워커스레드가 메인스레드가 호출하도록 넣는 Job
	JobQueue _jobs;
private:
	map<uint64, shared_ptr<Player>> _players;
	map<uint64, shared_ptr<Monster>> _monsters;
	map<uint64, shared_ptr<Arrow>> _arrows;
	map<uint32, Item> _items;
	vector<uint64>_deletearrowlist;
public:
	Tilemap _tilemap;
	const int32 DESIRED_COUNT = 30;
	vector<Sector>_gameRoomSector;//1차원배열로 y*height +x 형태로 쓸거임 캐시 효율을 위해  
	//Sector=>vector<Creature*>
	const int32 SECTOR_HEIGHT = 5;
	const int32 SECTOR_WIDTH = 7;
	const int32 dirX[9] = { 0, 1,0,0,1,-1,-1,1,-1 };
	const int32 dirY[9] = { 0,1,1,-1,-1,1,0,0,-1 };
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

