#include "pch.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Player.h"
#include "Creature.h"
#include "Monster.h"
#include "GameSession.h"
#include "Arrow.h"

shared_ptr<GameRoom> GRoom = make_shared<GameRoom>();

GameRoom::GameRoom()
{
	_deletearrowlist.reserve(63*43);
	_gameRoomSector.resize(SECTOR_HEIGHT * SECTOR_WIDTH);
	for (int i = 0; i < SECTOR_HEIGHT * SECTOR_WIDTH; ++i) {
		_gameRoomSector[i]._sectorArrows.reserve(25);
	}
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init()
{
	shared_ptr<GameRoom>gameRoom = shared_from_this();

	/*shared_ptr<Monster> monster = GameObject::CreateMonster();
	monster->info.set_posx(8);
	monster->info.set_posy(8);
	gameRoom->Enter(monster);

	Protocol::S_AddObject AddedMonster;
	*AddedMonster.add_objects() = monster->info;

	SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedMonster);
	gameRoom->Broadcast(sendBuf);*/

	//PushJob([gameRoom]() {
	//	shared_ptr<Monster> monster = GameObject::CreateMonster();
	//	monster->info.set_posx(8);
	//	monster->info.set_posy(8);
	//	gameRoom->Enter(monster);
	//
	//	Protocol::S_AddObject AddedMonster;
	//	*AddedMonster.add_objects() = monster->info;
	//
	//	SendBufferRef sendBuf= ServerPacketHandler::Make_S_AddObject(AddedMonster);
	//	gameRoom->Broadcast(sendBuf);
	//	//gameRoom->PushBroadcastJob(sendBuf);
	//	});


	_tilemap.LoadFile(L"C:\\Users\\서정원\\Desktop\\ServerClient\\ServerProject\\Server\\Client\\Resources\\Tilemap\\Tilemap_01.txt");


	//	shared_ptr<Monster> monster = GameObject::CreateMonster();
//	monster->info.set_posx(8);
//	monster->info.set_posy(8);
//	AddObject(monster);
}

void GameRoom::TickMonsterSpawn() {
	shared_ptr<GameRoom>gameRoom = shared_from_this();
	if (_monsters.size() < DESIRED_COUNT)
	{

		Vec2Int randPos = gameRoom->GetRandomEmptyCellPos();

		if (randPos == Vec2Int{ -1,-1 })return;
		//빈공간없으면 spawn x

		shared_ptr<Monster> monster = GameObject::CreateMonster();

		monster->info.set_posx(randPos.x);
		monster->info.set_posy(randPos.y);
	
		gameRoom->Enter(monster);

		Protocol::S_AddObject AddedMonster;
		*AddedMonster.add_objects() = monster->info;
		SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedMonster);
		gameRoom->BroadcastBySector(sendBuf, gameRoom->GetSectorPos(monster->info.posx(), monster->info.posy()));
	}
}

void GameRoom::DeleteProjectiles()
{
	for (uint64 idx : _deletearrowlist) {
		Leave(idx);
	}
	_deletearrowlist.clear();
}

Sector* GameRoom::GetSectorAt(Vec2Int sectorPos)
{
	if (!CheckValidSectorPos(sectorPos))return nullptr;


	return &_gameRoomSector[sectorPos.y *SECTOR_WIDTH+ sectorPos.x];
}

Vec2Int GameRoom::GetSectorPos(int32 x, int32 y)
{
	/*		0                          63
		0	[	][	][	][	][	][	][	] 480
			[	][	][	][	][	][	][	] 960
			[	][	][	][	][	][	][	] 1440
			[	][	][	][	][	][	][	] 1920
		43	[	][	][	][	][	][	][	] 2400 
			480 960 1440 1920 2400 2880 3360
	*/
	int32 sectorX = x / 10;
	int32 sectorY = y / 10;

	if(CheckValidSectorPos(Vec2Int(sectorX,sectorY)))
		return Vec2Int(sectorX, sectorY);

	return Vec2Int(-1, -1);
}

void GameRoom::InsertAtSector(Vec2Int sectorPos, GameObject* gameObject)
{
	//일단 만드려는건
	//섹터좌표를 입력하면 이게 현재 저장된 객체의 섹터좌표와 같으면 굳이 섹터 갱신 필요없음
	//그리고 다르다면 이전 섹터의 컨테이너에서 삭제하고 새로운 섹터 컨테이너에 추가하는 것 뿐

	if (gameObject == nullptr)return;

	if (gameObject->GetCurSectorPos() == sectorPos||!CheckValidSectorPos(sectorPos))return;

	int32 idx = sectorPos.y * SECTOR_WIDTH + sectorPos.x;
	
	DeleteFromSector(gameObject);

	gameObject->SetCurSectorPos(sectorPos);
	auto objectType = gameObject->info.objecttype();
	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_gameRoomSector[idx]._sectorPlayers.push_back(static_cast<Player*>(gameObject));
		gameObject->SetCurSectorIndex(_gameRoomSector[idx]._sectorPlayers.size() - 1);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_gameRoomSector[idx]._sectorMonsters.push_back(static_cast<Monster*>(gameObject));
		gameObject->SetCurSectorIndex(_gameRoomSector[idx]._sectorMonsters.size() - 1);
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
		_gameRoomSector[idx]._sectorArrows.push_back(static_cast<Arrow*>(gameObject));
		gameObject->SetCurSectorIndex(_gameRoomSector[idx]._sectorArrows.size() - 1);
		break;
	default:
		return;
	}
	

}

void GameRoom::DeleteFromSector(GameObject* gameObject)
{
	//이건 단순히 해당 객체가 현재 섹터좌표기준으로 빼내는것
	int32 sectoridx = gameObject->GetCurSectorPos().y * SECTOR_WIDTH + gameObject->GetCurSectorPos().x;

	if (!CheckValidSectorPos(gameObject->GetCurSectorPos()))return;

	auto objectType = gameObject->info.objecttype();
	uint64 idx = gameObject->GetCurSectorIndex();
	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
	{
		auto& players = _gameRoomSector[sectoridx]._sectorPlayers;//해당 배열 위치접근시 주소연산 줄이기위함(반복하면 레지스터가 알아서 )
		if (!players.empty()) {
			if (idx >= 0 && idx < players.size()) {
				Player* erasePlayer = players[idx];
				players[idx] = players.back();
				players[idx]->SetCurSectorIndex(idx);
				erasePlayer->SetCurSectorIndex(-1); //만약 .back()을 가리키는 원소와 idx랑 같을때 해당 섹터에서 제거하기 전 idx가 유효한 값으로 덮어씌워져서 이런 구조로
				players.pop_back();
			}
		}
	}
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
	{
		auto& monsters = _gameRoomSector[sectoridx]._sectorMonsters;//해당 배열 위치접근시 주소연산 줄이기위함(반복하면 레지스터가 알아서 )
		if (!monsters.empty()) {
			if (idx >= 0 && idx < monsters.size()) {
				Monster* eraseMonster = monsters[idx];
				monsters[idx] = monsters.back();
				monsters[idx]->SetCurSectorIndex(idx);
				eraseMonster->SetCurSectorIndex(-1); //만약 .back()을 가리키는 원소와 idx랑 같을때 해당 섹터에서 제거하기 전 idx가 유효한 값으로 덮어씌워져서 이런 구조로
				monsters.pop_back();
			}

		}
	}
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
	{
		auto& arrows = _gameRoomSector[sectoridx]._sectorArrows;//해당 배열 위치접근시 주소연산 줄이기위함(반복하면 레지스터가 알아서 )
		if (!arrows.empty()) {
			if (idx >= 0 && idx < arrows.size()) {
				Arrow* eraseArrow = arrows[idx];
				arrows[idx] = arrows.back();
				arrows[idx]->SetCurSectorIndex(idx);
				eraseArrow->SetCurSectorIndex(-1); //만약 .back()을 가리키는 원소와 idx랑 같을때 해당 섹터에서 제거하기 전 idx가 유효한 값으로 덮어씌워져서 이런 구조로
				arrows.pop_back();
			}

		}
	}
		break;
	default:
		break;
	}


	

}

bool GameRoom::CheckValidSectorPos(Vec2Int sectorPos)
{
	if (sectorPos.x < 0 || sectorPos.x >= SECTOR_WIDTH || sectorPos.y < 0 || sectorPos.y >= SECTOR_HEIGHT)
		return false;

	return true;
}


bool GameRoom::IsSameSector(Vec2Int FirstCellPos, Vec2Int SecondCellPos)
{
	Vec2Int FirstSectorPos = GetSectorPos(FirstCellPos.x, FirstCellPos.y);
	Vec2Int SecondSectorPos = GetSectorPos(SecondCellPos.x, SecondCellPos.y);

	if (FirstSectorPos == Vec2Int(-1, -1) || SecondSectorPos == Vec2Int(-1, -1))return false;

	if (FirstSectorPos == SecondSectorPos)return true;


	return false;
}

bool GameRoom::CanGoBySector(Vec2Int cellPos)
{
	Tile* tile = _tilemap.GetTileAt(cellPos);
	if (tile == nullptr)
		return false;

	if (GetCreatureAtSector(cellPos) != nullptr)
		return false;


	return tile->value != 1;
}

Player* GameRoom::GetPlayerAtSector(Vec2Int cellPos)
{
	Vec2Int SectorPos = GetSectorPos(cellPos.x, cellPos.y);
	
	if (!CheckValidSectorPos(SectorPos)) return nullptr;

	Sector* sector = GetSectorAt(SectorPos);
	
	if (sector) {
		for (auto pl : sector->_sectorPlayers) {
			if (pl->GetCellPos() == cellPos) {
				return pl;
			}
		}
	}

	return nullptr;
}

Monster* GameRoom::GetMonsterAtSector(Vec2Int cellPos)
{
	Vec2Int SectorPos = GetSectorPos(cellPos.x, cellPos.y);

	if (!CheckValidSectorPos(SectorPos))return nullptr;

		Sector* sector = GetSectorAt(SectorPos);

	if (sector) {
		for (auto mon : sector->_sectorMonsters) {
			if (mon->GetCellPos() == cellPos) {
				return mon;
			}
		}
	}

	return nullptr;
}

Arrow* GameRoom::GetArrowAtSector(Vec2Int cellPos )
{
	Vec2Int SectorPos = GetSectorPos(cellPos.x, cellPos.y);

	if (!CheckValidSectorPos(SectorPos))return nullptr;

		Sector* sector = GetSectorAt(SectorPos);

	if (sector) {
		for (auto arr : sector->_sectorArrows) {
			if (arr->GetCellPos() == cellPos) {
				return arr;
			}
		}
	}

	return nullptr;
}

Creature* GameRoom::GetCreatureAtSector(Vec2Int cellPos)
{

	Vec2Int SectorPos = GetSectorPos(cellPos.x, cellPos.y);

	if (!CheckValidSectorPos(SectorPos))return nullptr;

	Sector* sector = GetSectorAt(SectorPos);

	if (sector) {
		for (auto mon : sector->_sectorMonsters) {
			if (mon->GetCellPos() == cellPos) {
				return mon;
			}
		}
	}

	if (sector) {
		for (auto pl : sector->_sectorPlayers) {
			if (pl->GetCellPos() == cellPos) {
				return pl;
			}
		}
	}

	return nullptr;
}

weak_ptr<Player> GameRoom::FindClosestPlayerBySector(Vec2Int cellPos)
{
	Vec2Int curSector = GetSectorPos(cellPos.x, cellPos.y);

	weak_ptr<Player>ret;

	if (curSector == Vec2Int(-1, -1))return ret;

	float best = FLT_MAX;


	for (int i = 0; i < 9; ++i) {
		Vec2Int nextFindSectorPos{ curSector.x + dirX[i],curSector.y + dirY[i] };
		if (CheckValidSectorPos(nextFindSectorPos)) {
			Sector* nextFindSector = GetSectorAt(nextFindSectorPos);
			if (nextFindSector) {
				for (Player* pl : nextFindSector->_sectorPlayers) {
					float dist = Vec2Int(pl->GetCellPos() - cellPos).LengthSquared();
					if (dist < best) {
						best = dist;
						ret = static_pointer_cast<Player>(pl->shared_from_this());
					}
				}
			}
		}
	}

	return ret;
}

void GameRoom::DoSomethingCrossingSectors(Vec2Int curCellPos, Vec2Int LastCellPos, function<void(Vec2Int)> add, function<void(Vec2Int)>remove)
{
	Vec2Int CurSectorPos = GetSectorPos(curCellPos.x, curCellPos.y);
	Vec2Int LastSectorPos = GetSectorPos(LastCellPos.x, LastCellPos.y);

	const int32 dirX = CurSectorPos.x - LastSectorPos.x;
	const int32 dirY = CurSectorPos.y - LastSectorPos.y;

	Vec2Int RemoveSectorMidPos = LastSectorPos - Vec2Int(dirX, dirY);
	Vec2Int AddSectorMidPos = CurSectorPos + Vec2Int(dirX, dirY);

	if (dirX != 0) {//오른쪽 또는 왼쪽 섹터 이동은  -> 위아래 수행
		for (int i = -1; i <= 1; ++i) {
			remove(Vec2Int(RemoveSectorMidPos.x, RemoveSectorMidPos.y + i));
			add(Vec2Int(AddSectorMidPos.x, AddSectorMidPos.y + i));
		}
	}
	else {//위 아래 섹터 이동-> 왼 오 수행
		for (int i = -1; i <= 1; ++i) {
			remove(Vec2Int(RemoveSectorMidPos.x+i, RemoveSectorMidPos.y ));
			add(Vec2Int(AddSectorMidPos.x+i, AddSectorMidPos.y));
		}
	}

	
}



void GameRoom::Update()
{
	//uint64 start = GetTickCount64();
	for (auto& item : _players)
	{
		item.second->Update();
	}
	for (auto& item : _monsters)
	{
		item.second->Update();
	}
	for (auto& item : _arrows)
	{
		item.second->Update();
	}
	TickMonsterSpawn();
	DeleteProjectiles();
	

	//uint64 end = GetTickCount64();
	//cout << "[Update Time] " << (end - start) << " ms" << endl;
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
	{
		auto findIt = _arrows.find(id);
		if (findIt != _arrows.end())
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
		InsertAtSector(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()), static_cast<Creature*>(gameObject.get()));
		gameObject->SetCurSectorPos(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()));
		_players[id] = static_pointer_cast<Player>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		InsertAtSector(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()), static_cast<Creature*>(gameObject.get()));
		gameObject->SetCurSectorPos(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()));
		_monsters[id] = static_pointer_cast<Monster>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
		//화살을 섹터로 현재 관리할 필요없어보임-> 섹터로 관리해서 양쪽 플레이어 및 화살 둘다 섹터 이동 시에만 판단하여 비용 감소

		InsertAtSector(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()), static_cast<Creature*>(gameObject.get()));
		gameObject->SetCurSectorPos(GetSectorPos(gameObject->info.posx(), gameObject->info.posy()));
		_arrows[id] = static_pointer_cast<Arrow>(gameObject);
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
		DeleteFromSector(gameObject.get());
		_players.erase(id);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:	
		DeleteFromSector(gameObject.get());
		_monsters.erase(id);
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
		DeleteFromSector(gameObject.get());
		_arrows.erase(id);
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

void GameRoom::BroadcastBySector(SendBufferRef sendBuffer, Vec2Int SectorPos)
{
	if (SectorPos == Vec2Int(-1, -1))return;

	for (int i = 0; i < 9; ++i) {
		Vec2Int nextFindSectorPos{ SectorPos.x + dirX[i],SectorPos.y + dirY[i] };
		if (CheckValidSectorPos(nextFindSectorPos)) {
			Sector* nextFindSector = GetSectorAt(nextFindSectorPos);
			for (Player* pl : nextFindSector->_sectorPlayers) {
				pl->session->Send(sendBuffer);			
			}
		}
	}

}

void GameRoom::AddItem(Item item)
{
	_items[item.GetItemID()] = item;
}

void GameRoom::DeleteItem(uint32 id)
{
	_items.erase(id);
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

vector<Protocol::ObjectInfo> GameRoom::GetRoomArrowInfo()
{
	vector<Protocol::ObjectInfo> infos;

	for (auto m : _arrows)
		infos.push_back(m.second->info);

	return infos;
}

vector<Protocol::ItemInfo> GameRoom::GetRoomItemInfo()
{
	vector<Protocol::ItemInfo> infos;

	for (auto m : _items)
		infos.push_back(m.second.itemInfo);

	return infos;
}

//astar 컨텐츠 코드





shared_ptr<Player>  GameRoom::FindClosestPlayer(Vec2Int pos) {
	float best = FLT_MAX;
	shared_ptr<Player> ret = nullptr;

	for (auto& items : _players)
	{
		shared_ptr<Player> player = items.second;
		if (player)
		{
			Vec2Int dir = pos - player->GetCellPos();
			float dist = dir.LengthSquared();
			if (dist < best)
			{
				best = dist;
				ret = player;
			}
		}
	}

	return ret;
}

bool  GameRoom::FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth) {
	
	
	//int32 depth = abs(src.y - dest.y) + abs(src.x - dest.x);
	//if (depth >= maxDepth)
	//	return false;

	priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
	map<Vec2Int, int32> best;
	map<Vec2Int, Vec2Int> parent;

	// 초기값
	{
		int32 cost = abs(dest.y - src.y) + abs(dest.x - src.x);

		pq.push(PQNode(cost, src));
		best[src] = cost;
		parent[src] = src;
	}

	Vec2Int front[4] =
	{
		{0, -1},
		{0, 1},
		{-1, 0},
		{1, 0},
	};

	bool found = false;

	while (pq.empty() == false)
	{
		// 제일 좋은 후보를 찾는다
		PQNode node = pq.top();
		pq.pop();

		// 더 짧은 경로를 뒤늦게 찾았다면 스킵
		if (best[node.pos] < node.cost)
			continue;

		// 목적지에 도착했으면 바로 종료
		if (node.pos == dest)
		{
			found = true;
			break;
		}

		// 방문
		for (int32 dir = 0; dir < 4; dir++)
		{
			Vec2Int nextPos = node.pos + front[dir];

			if (CanGoBySector(nextPos) == false)
				continue;

			int32 depth = abs(src.y - nextPos.y) + abs(src.x - nextPos.x);
			if (depth >= maxDepth)
				continue;

			int32 cost = abs(dest.y - nextPos.y) + abs(dest.x - nextPos.x);
			int32 bestValue = best[nextPos];
			if (bestValue != 0)
			{
				// 다른 경로에서 더 빠른 길을 찾았으면 스킵
				if (bestValue <= cost)
					continue;
			}

			// 예약 진행
			best[nextPos] = cost;
			pq.push(PQNode(cost, nextPos));
			parent[nextPos] = node.pos;
		}
	}

	if (found == false)
	{
		float bestScore = FLT_MAX;

		for (auto& item : best)
		{
			Vec2Int pos = item.first;
			int32 score = item.second;

			// 동점이라면, 최초 위치에서 가장 덜 이동하는 쪽으로
			if (bestScore == score)
			{
				int32 dist1 = abs(dest.x - src.x) + abs(dest.y - src.y);
				int32 dist2 = abs(pos.x - src.x) + abs(pos.y - src.y);
				if (dist1 > dist2)
					dest = pos;
			}
			else if (bestScore > score)
			{
				dest = pos;
				bestScore = score;
			}
		}
	}

	path.clear();
	Vec2Int pos = dest;

	while (true)
	{
		path.push_back(pos);

		// 시작점
		if (pos == parent[pos])
			break;

		pos = parent[pos];
	}

	std::reverse(path.begin(), path.end());
	return true;
}

bool GameRoom::MyFindPath(Vec2Int start, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth)
{//f=g+h
	priority_queue<MyPQNode, vector<MyPQNode>, greater<MyPQNode>> pq;
	map<Vec2Int, int32> bestbyfar;
	map<Vec2Int, Vec2Int> parent;

	auto heuristic = [](const Vec2Int& a, const Vec2Int& b) {
		return abs(b.x - a.x) + abs(b.y - a.y); // 맨해튼 거리
		};

	// 초기화
	bestbyfar[start] = 0;
	parent[start] = start;
	pq.push({ heuristic(start, dest), 0, start });//f,g,pos

	//시작점은 g가 0이므로 f=g+h에서 h만 남아 heuristic만 반영 g는 0 

	Vec2Int dirs[4] = { {0,1},{0,-1},{1,0},{-1,0} };
	bool found = false;

	// maxDepth: 너무 작으면 돌아가는 길 포기하므로, 최소 보장
	int heuristicDist = heuristic(start, dest);
	if (maxDepth < heuristicDist * 4)  // 여유를 충분히 준다
		maxDepth = heuristicDist * 4;

	while (!pq.empty())
	{
		MyPQNode cur = pq.top();
		pq.pop();

		if (cur.pos == dest)
		{
			found = true;
			break;
		}

		if (cur.g > bestbyfar[cur.pos])
			continue;

		for (int i = 0; i < 4; i++)
		{
			Vec2Int next = cur.pos + dirs[i];
			if (!CanGoBySector(next)) continue;

			int nextG = cur.g + 1;
			if (nextG > maxDepth) continue;

			if (bestbyfar.find(next) == bestbyfar.end() || nextG < bestbyfar[next])
			{
				bestbyfar[next] = nextG;

				// Weighted A* : h에 가중치 (조금 더 목적지 쪽으로 치우치게)
				int h = heuristic(next, dest);
				int f = nextG + h * 2; // W = 2

				pq.push({ f, nextG, next });
				parent[next] = cur.pos;
			}
		}
	}

	// 목적지에 도달 못 했을 때 fallback
	if (!found)
	{
		int bestH = INT_MAX;
		Vec2Int fallback = start;

		for (auto& item : bestbyfar)
		{
			Vec2Int pos = item.first;
			int g = item.second;
			int h = heuristic(pos, dest);

			// h가 더 작거나, 동점이면 g가 더 큰 쪽 선택 (멀리 간 후보 선호)
			if (h < bestH || (h == bestH && g > bestbyfar[fallback]))
			{
				bestH = h;
				fallback = pos;
			}
		}
		dest = fallback;
	}

	// 경로 복원
	path.clear();
	Vec2Int pos = dest;
	while (true)
	{
		path.push_back(pos);
		if (pos == parent[pos]) break;
		pos = parent[pos];
	}
	reverse(path.begin(), path.end());

	return true;
}

//bool GameRoom::MyFindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth)
//{
//
//	priority_queue<MyPQNode, vector<MyPQNode>, greater<MyPQNode>>pq;
//	map<Vec2Int, int32> gCost; //시작점으로부터 각 노드까지 최단 비용
//	map<Vec2Int, Vec2Int> parent;
//
//	//
//	auto heuristic = [](Vec2Int& a, Vec2Int& b) {
//		return abs(b.x - a.x) + abs(b.y - a.y);
//		};
//	
//	gCost[src] = 0;
//	int h = heuristic(src, dest);
//	parent[src] = src;
//	pq.push({ h,0,src });
//
//	Vec2Int dir[4] = { {0,1},{0,-1},{1,0},{-1,0} };
//	int cost[] = { 1,1,1,1 };
//	bool found = false; //실제 경로가 없을수도 있으니 찾았는지 여부 따지기 용도
//
//	while (!pq.empty())
//	{
//		MyPQNode cur = pq.top();
//		pq.pop();
//
//		if (cur.pos == dest) {
//			found = true;
//			break;
//		}
//
//		if (cur.g > gCost[cur.pos])//현재 노드 기준 시작점으로부터 비용이 지금껏 방문한 현재 노드 비용보다 크다면 넘김
//			continue;
//
//		for (int i = 0; i < 4; ++i) {
//			Vec2Int next = cur.pos + dir[i];
//
//			if (!CanGo(next))continue; //현재 좌표 기준 4방향을 순차로 갈때 못가는 좌표면 넘김
//
//			int nextG = cur.g + cost[i]; // 시작점에서 현재좌표까지 최소 비용 + cost(1)
//			if (nextG >= maxDepth)continue; //시작점에서 이 노드까지 거리가 maxDepth(10)보다 크면 탐색 그만
//
//			if (gCost.find(next) == gCost.end() || nextG < gCost[next])
//			{
//				gCost[next] = nextG;
//				int f = nextG + heuristic(next, dest);
//				pq.push({ f,nextG,next });
//				parent[next] = cur.pos;
//			}
//		}
//	}
//	if (!found) {
//		float bestScore = FLT_MAX;
//		Vec2Int fallback = src;
//
//		for (auto& item : gCost) {
//			Vec2Int pos = item.first;
//			int32 g = item.second;
//			int32 h = heuristic(pos, dest);
//			int32 score = g + h;
//
//			//if (bestScore == score) {
//			//	// 동점이면 시작점에서 더 가까운 쪽 선택
//			//	int32 dist1 = abs(dest.x - src.x) + abs(dest.y - src.y);
//			//	int32 dist2 = abs(pos.x - src.x) + abs(pos.y - src.y);
//			//	if (dist2 < dist1)
//			//		fallback = pos;
//			//}
//			//else if (score < bestScore) {
//			//	fallback = pos;
//			//	bestScore = score;
//			//}
//			if (score < bestScore || (score == bestScore && g > gCost[fallback])) {
//				bestScore = score;
//				fallback = pos;
//			}
//		}
//		dest = fallback;
//	}
//	path.clear();
//	Vec2Int pos = dest;
//	while (true)
//	{
//		path.push_back(pos);
//		if (pos == parent[pos])break;
//		pos = parent[pos];
//	}
//	reverse(path.begin(), path.end());
//	return true;
//}

bool  GameRoom::CanGo(Vec2Int cellPos) {
	
	Tile* tile = _tilemap.GetTileAt(cellPos);
	if (tile == nullptr)
		return false;

	if (GetCreatureAt(cellPos) != nullptr)
		return false;


	return tile->value != 1;
}

Vec2Int  GameRoom::GetRandomEmptyCellPos() {
	Vec2Int ret = { -1, -1 };

	Vec2Int size = _tilemap.GetMapSize();
	mt19937 rng{ random_device{}() };
	uniform_int_distribution<int>distX(0, (int)size.x);
	uniform_int_distribution<int>distY(0, (int)size.y);
	// 몇 번 시도?
	for (int i = 0; i < 1000; ++i) {
		int32 x = distX(rng);
		int32 y = distY(rng);
		Vec2Int cellPos{ x, y };

		if (CanGoBySector(cellPos))
			return cellPos;
	}
	return ret;
}

shared_ptr<GameObject>  GameRoom::GetGameObjectAt(Vec2Int cellPos) {
	//Projectile은 충돌 여부에 당락을 주지않게 포함 x
	for (auto& item : _players)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	for (auto& item : _monsters)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}
	return nullptr;
}


shared_ptr<Creature>  GameRoom::GetCreatureAt(Vec2Int cellPos) {
	//Projectile은 충돌 여부에 당락을 주지않게 포함 x
	for (const auto& item : _players)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	for (const auto& item : _monsters)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}
	return nullptr;
}

optional<Item> GameRoom::GetItemAt(Vec2Int cellPos)
{
	for (auto& item : _items)
	{
		if (item.second.GetCellPos() == cellPos)
			return item.second;
	}
	return nullopt;
}

void GameRoom::AddDeleteProjectiletoList(uint64 idx)
{
	_deletearrowlist.emplace_back(idx);
	//제거를 update에서하면 이터레이터 무효화때문에 update이후 제거 위한 컨테이너에 추가
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
//	//서로의 존재를 연결
//	session->gameRoom = shared_from_this();
//	session->player = curPlayer;
//	curPlayer->session = session;
//
//	curPlayer->info.set_posx(5);
//	curPlayer->info.set_posy(5);
//	
//
//	//입장한 클라에게 정보를 보내주기
//	{
//		SendBufferRef sendBuffer=ServerPacketHandler::Make_S_MyPlayer(curPlayer->info);
//		session->Send(sendBuffer);
//	}
//	//모든 오브젝트의 정보 전송
//	{
//		Protocol::S_AddObject pkt;
//
//		for (auto& item : _players)
//		{
//			//read - pkt.object(1) index
//			//write - Protocol::ObjectInfo* info =  pkt.add_objects() -pointer 반환
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
//	//TODO Validation 해킹 체킹
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
//		*info = gameObject->info; //현재 추가될 애 정보
//		
//		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
//		//session->Send(sendBuffer); 모든 애들한테 보내줘야함
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
//	//TODO 오브젝트 삭제 메시지 전송
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
