#include "pch.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Player.h"
#include "Creature.h"
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
		//gameRoom->PushBroadcastJob(sendBuf);
		});


	_tilemap.LoadFile(L"C:\\Users\\������\\Desktop\\ServerClient\\ServerProject\\Server\\Client\\Resources\\Tilemap\\Tilemap_01.txt");
//	shared_ptr<Monster> monster = GameObject::CreateMonster();
//	monster->info.set_posx(8);
//	monster->info.set_posy(8);
//	AddObject(monster);
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

//astar ������ �ڵ�





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
				dist = best;
				ret = player;
			}
		}
	}

	return ret;
}

bool  GameRoom::FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth) {
	
	
	int32 depth = abs(src.y - dest.y) + abs(src.x - dest.x);
	if (depth >= maxDepth)
		return false;

	priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
	map<Vec2Int, int32> best;
	map<Vec2Int, Vec2Int> parent;

	// �ʱⰪ
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
		// ���� ���� �ĺ��� ã�´�
		PQNode node = pq.top();
		pq.pop();

		// �� ª�� ��θ� �ڴʰ� ã�Ҵٸ� ��ŵ
		if (best[node.pos] < node.cost)
			continue;

		// �������� ���������� �ٷ� ����
		if (node.pos == dest)
		{
			found = true;
			break;
		}

		// �湮
		for (int32 dir = 0; dir < 4; dir++)
		{
			Vec2Int nextPos = node.pos + front[dir];

			if (CanGo(nextPos) == false)
				continue;

			int32 depth = abs(src.y - nextPos.y) + abs(src.x - nextPos.x);
			if (depth >= maxDepth)
				continue;

			int32 cost = abs(dest.y - nextPos.y) + abs(dest.x - nextPos.x);
			int32 bestValue = best[nextPos];
			if (bestValue != 0)
			{
				// �ٸ� ��ο��� �� ���� ���� ã������ ��ŵ
				if (bestValue <= cost)
					continue;
			}

			// ���� ����
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

			// �����̶��, ���� ��ġ���� ���� �� �̵��ϴ� ������
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

		// ������
		if (pos == parent[pos])
			break;

		pos = parent[pos];
	}

	std::reverse(path.begin(), path.end());
	return true;
}

bool GameRoom::MyFindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth)
{
	priority_queue<MyPQNode, vector<MyPQNode>, greater<MyPQNode>> pq;
	map<Vec2Int, int32> gCost;
	map<Vec2Int, Vec2Int> parent;

	auto heuristic = [](const Vec2Int& a, const Vec2Int& b) {
		return abs(b.x - a.x) + abs(b.y - a.y); // ����ư �Ÿ�
		};

	// �ʱ�ȭ
	gCost[src] = 0;
	parent[src] = src;
	pq.push({ heuristic(src, dest), 0, src });

	Vec2Int dirs[4] = { {0,1},{0,-1},{1,0},{-1,0} };
	bool found = false;

	// maxDepth: �ʹ� ������ ���ư��� �� �����ϹǷ�, �ּ� ����
	int heuristicDist = heuristic(src, dest);
	if (maxDepth < heuristicDist * 4)  // ������ ����� �ش�
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

		if (cur.g > gCost[cur.pos])
			continue;

		for (int i = 0; i < 4; i++)
		{
			Vec2Int next = cur.pos + dirs[i];
			if (!CanGo(next)) continue;

			int nextG = cur.g + 1;
			if (nextG > maxDepth) continue;

			if (gCost.find(next) == gCost.end() || nextG < gCost[next])
			{
				gCost[next] = nextG;

				// Weighted A* : h�� ����ġ (���� �� ������ ������ ġ��ġ��)
				int h = heuristic(next, dest);
				int f = nextG + h * 2; // W = 2

				pq.push({ f, nextG, next });
				parent[next] = cur.pos;
			}
		}
	}

	// �������� ���� �� ���� �� fallback
	if (!found)
	{
		int bestH = INT_MAX;
		Vec2Int fallback = src;

		for (auto& item : gCost)
		{
			Vec2Int pos = item.first;
			int g = item.second;
			int h = heuristic(pos, dest);

			// h�� �� �۰ų�, �����̸� g�� �� ū �� ���� (�ָ� �� �ĺ� ��ȣ)
			if (h < bestH || (h == bestH && g > gCost[fallback]))
			{
				bestH = h;
				fallback = pos;
			}
		}
		dest = fallback;
	}

	// ��� ����
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
//	map<Vec2Int, int32> gCost; //���������κ��� �� ������ �ִ� ���
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
//	bool found = false; //���� ��ΰ� �������� ������ ã�Ҵ��� ���� ������ �뵵
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
//		if (cur.g > gCost[cur.pos])//���� ��� ���� ���������κ��� ����� ���ݲ� �湮�� ���� ��� ��뺸�� ũ�ٸ� �ѱ�
//			continue;
//
//		for (int i = 0; i < 4; ++i) {
//			Vec2Int next = cur.pos + dir[i];
//
//			if (!CanGo(next))continue; //���� ��ǥ ���� 4������ ������ ���� ������ ��ǥ�� �ѱ�
//
//			int nextG = cur.g + cost[i]; // ���������� ������ǥ���� �ּ� ��� + cost(1)
//			if (nextG >= maxDepth)continue; //���������� �� ������ �Ÿ��� maxDepth(10)���� ũ�� Ž�� �׸�
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
//			//	// �����̸� ���������� �� ����� �� ����
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

	// �� �� �õ�?
	while (true)
	{
		int32 x = rand() % size.x;
		int32 y = rand() % size.y;
		Vec2Int cellPos{ x, y };

		if (CanGo(cellPos))
			return cellPos;
	}
}

shared_ptr<GameObject>  GameRoom::GetGameObjectAt(Vec2Int cellPos) {
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
