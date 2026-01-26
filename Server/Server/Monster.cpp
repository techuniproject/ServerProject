#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "Player.h"
#include "Arrow.h"
#include "GameSession.h"



Monster::Monster()
{
	info.set_name("MonsterName");
	info.set_hp(100);
	info.set_maxhp(100);
	info.set_attack(1);
	info.set_defence(0); //나중엔 data sheet으로 읽어오는 방식

	item.SetAliveState(false);

	_findMaxDist = 10;
}

Monster::~Monster()
{
	
}

void Monster::Init()
{

}

void Monster::Update()//어차피 메인스레드 로직이라 lock신경X, send도 메인이 해도되긴함
{
	//여기서 호출하는 로직은 Job으로 처리안해도됨 메인스레드가 함
	Super::Update();
}

//void Monster::UpdateIdle()
//{
//
//	if (!room) return;
//
//	// 타겟 없으면 새로 탐색
//	if (_target.lock() == nullptr)
//		_target = room->FindClosestPlayer(GetCellPos());
//	shared_ptr<Player> player = _target.lock();
//	if (!player) return;
//
//	// =============================
//	// 1) 공격 범위 판정
//	// =============================
//	Vec2Int d = player->GetCellPos() - GetCellPos();
//	int dist = abs(d.x) + abs(d.y);
//	if (dist <= 1) // 붙어있으면 바로 공격
//	{
//		SetDir(GetLookAtDir(player->GetCellPos()));
//		SetState(SKILL, true);
//		_waitUntil = GetTickCount64() + 1000; // 공격 쿨다운
//		return;
//	}
//
//	// =============================
//	// 2) 경로 탐색 (쿨다운 적용)
//	// =============================
//	uint64 now = GetTickCount64();
//	if (now > _lastPathUpdate + _pathUpdateInterval)
//	{
//		_path.clear();
//		if (room->MyFindPath(GetCellPos(), player->GetCellPos(), OUT _path))
//		{
//			_lastPathUpdate = now;
//		}
//	}
//
//	// =============================
//	// 3) 경로 따라가기
//	// =============================
//	if (_path.size() > 1)
//	{
//		Vec2Int nextPos = _path[1];
//		if (room->CanGo(nextPos))
//		{
//			SetDir(GetLookAtDir(nextPos));
//			SetCellPos(nextPos);
//			SetState(MOVE, true);
//			_waitUntil = now + 200; // 이동 텀
//			_path.erase(_path.begin()); // 첫 칸 소비
//		}
//	}
//}



void Monster::UpdateIdle()
{
	
	if (room == nullptr)
		return;

	
	// Find Player
	if (_target == nullptr)
		_target = room->FindClosestPlayerBySector(GetCellPos());

	//shared_ptr<Player> player = _target.lock();

	if (_target)
	{
		Vec2Int dir = _target->GetCellPos() - GetCellPos();
		int32 dist = abs(dir.x) + abs(dir.y);
		if (dist == 1)
		{
			SetDir(GetLookAtDir(_target->GetCellPos()));
			SetState(SKILL);
			BroadcastMoveBySector();
			//_waitUntil = GetTickCount64() + 500; //+1초
		}
		else
		{
			if (dist >= _findMaxDist) {
				_target = nullptr;
				return;
			}
			vector<Vec2Int> path;
			if (room->FindPath(GetCellPos(), _target->GetCellPos(), OUT path,_findMaxDist))
			{
				if (path.size() > 1)
				{
					Vec2Int nextPos = path[1];
					Vec2Int LastSectorPos = room->GetSectorPos(info.posx(), info.posy());
					Vec2Int CurSectorPos = room->GetSectorPos(nextPos.x, nextPos.y);
					if (room->CanGoBySector(nextPos))
					{
						SetDir(GetLookAtDir(nextPos));
						SetCellPos(nextPos);
						
						auto UpdateSectorPacket = [&](Vec2Int sectorPos, bool isSpawn) {
							Sector* sector = room->GetSectorAt(sectorPos);
							if (sector) {
								for (Player* pl : sector->_sectorPlayers) {
									if (isSpawn) {
										if (pl && pl != _target) {//타겟이면 이미 마주쳐서 생성됨 플레이어 C_Move처리에서 추가리스트에 추가

											Protocol::S_AddObject pkt;
											*pkt.add_objects() = info;
											SendBufferRef AddMeToOtherBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
											pl->session->Send(AddMeToOtherBuffer);
										}
									}
									else {
										if (pl && pl != _target) {

											Protocol::S_RemoveObject pkt;
											pkt.add_ids(GetObjectID());
											SendBufferRef RemoveMeToOtherBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
											pl->session->Send(RemoveMeToOtherBuffer);
										}
									}
								}
							}
							};

						if(!isSameSector(room->GetSectorPos(nextPos.x,nextPos.y))){
							//몬스터가 다른 플레이어 추적하면서 다른 섹터로 갔을때 다른 플레이어 눈에도 들어가야하니까
							room->InsertAtSector(room->GetSectorPos(nextPos.x, nextPos.y), static_cast<GameObject*>(shared_from_this().get()));
							SetCurSectorPos(room->GetSectorPos(nextPos.x, nextPos.y));
							const int32 dirX = CurSectorPos.x - LastSectorPos.x;
							const int32 dirY = CurSectorPos.y - LastSectorPos.y; //상하좌우만 이동되므로 둘중 하나는 0
							LastSectorPos.x -= dirX;
							LastSectorPos.y -= dirY;//가려고 말한 방향 반대로 해서 영향권없는쪽
							CurSectorPos.x += dirX;
							CurSectorPos.y += dirY;
							switch (dirX) {
							case 1: //오른쪽 
								for (int i = -1; i <= 1; ++i) {//영향권 아닌 애들 리스트부터
									UpdateSectorPacket(Vec2Int(LastSectorPos.x, LastSectorPos.y + i), false);//remove;
									UpdateSectorPacket(Vec2Int(CurSectorPos.x, CurSectorPos.y + i), true);//add;
								}
								break;
							case -1: //왼쪽
								for (int i = -1; i <= 1; ++i) {//영향권 아닌 애들 리스트부터                      
									UpdateSectorPacket(Vec2Int(LastSectorPos.x, LastSectorPos.y + i), false);//remove;
									UpdateSectorPacket(Vec2Int(CurSectorPos.x, CurSectorPos.y + i), true);//add
								}
								break;
							case 0:
								if (dirY == 1) {
									for (int i = -1; i <= 1; ++i) {//영향권 아닌 애들 리스트부터
										UpdateSectorPacket(Vec2Int(LastSectorPos.x + i, LastSectorPos.y), false);//remove;
										UpdateSectorPacket(Vec2Int(CurSectorPos.x + i, CurSectorPos.y), true);//add
									}
								}
								else if (dirY == -1) {
									for (int i = -1; i <= 1; ++i) {//영향권 아닌 애들 리스트부터
										UpdateSectorPacket(Vec2Int(LastSectorPos.x + i, LastSectorPos.y), false);//remove;
										UpdateSectorPacket(Vec2Int(CurSectorPos.x + i, CurSectorPos.y), true);//add

									}
								}
								break;
							}
							
						}

						_waitUntil = GetTickCount64() + 500; //+1초
						SetState(MOVE);
						BroadcastMoveBySector();
					}
				}
				else {//제자리일때 -> 이거 나중에 astar분석하고 수정
					// 제미나이는 이 코드 오히려 오류유발 -> 불필요하므로 target이나 없애라
					//SetCellPos(path[0]);
					//room->InsertAtSector(room->GetSectorPos(path[0].x, path[0].y), static_cast<GameObject*>(shared_from_this().get()));
					//SetCurSectorPos(room->GetSectorPos(path[0].x, path[0].y));
					_target = nullptr;
					SetState(IDLE);
				}
			}
		}
	}
}

void Monster::UpdateMove()
{
	uint64 now = GetTickCount64();

	if (_waitUntil > now)
		return;
	
	//SetState(IDLE,true);
	SetState(IDLE);
	BroadcastMoveBySector();
}

void Monster::UpdateSkill()
{
	int64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	Player* pl = GRoom->GetPlayerAtSector(GetFrontCellPos());
	if (pl) {
		pl->OnDamaged(static_pointer_cast<Creature>(shared_from_this()));
		_waitUntil = GetTickCount64() + 1000;
		//pl->SetState(HIT, true);
		pl->SetState(HIT);
		BroadcastMoveBySector();
	}
	//shared_ptr<Creature> creature=GRoom->GetCreatureAt(GetFrontCellPos());
	//
	//if (creature) {
	//	creature->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
	//	_waitUntil = GetTickCount64() + 1000;
	//	creature->SetState(HIT, true);
	//	//플레이어 맞았을 때 보류
	//}

	//SetState(IDLE,true);//차이?
	SetState(IDLE);
	BroadcastMoveBySector();
}

void Monster::UpdateHit()
{
	int64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	//SetState(IDLE,true);
	SetState(IDLE);
	BroadcastMoveBySector();
}

void Monster::ApplyHitStun(uint32 ms) {
	const uint64 now = GetTickCount64();
	// 재피격 시 HIT 연장: max 사용 (리셋 원하면 그냥 대입)
	_waitUntil = std::max<uint64>(_waitUntil, now + static_cast<uint64>(ms));

	//SetState(HIT, true);
	SetState(HIT);
	BroadcastMoveBySector();
}

bool Monster::OnDamaged(shared_ptr<Creature> attacker)
{
	shared_ptr<Monster>life_guard = static_pointer_cast<Monster>(shared_from_this());
	//생명늘리고 이후 OnDamaged의 수명주기 컨테이너에서 삭제하고 ->나머지 아이템 로직 수행후 이 함수끝나면 수명 0
	//Super::OnDamaged에서 피를깎아야 여기 로직에서 Hp==0이 수행되어 아이템 생성되니까 이게 최선
	bool isalive = Super::OnDamaged(attacker);
	//여기서 제거하면 미리 삭제된 상태로 다음 GetObjectHp에서 null crash
	

	if (attacker == nullptr)
		return false;

      	if (GetObjectHp() == 0) {
		if (GRoom) {
			item.SetAliveState(true);
			//auto ifarrow = dynamic_pointer_cast<Arrow>(attacker);
			
			if (attacker->info.objecttype()==Protocol::OBJECT_TYPE_PROJECTILE) {
				//item.SetBelongingId(ifarrow->GetBelongingId());
				item.SetBelongingId(static_pointer_cast<Arrow>(attacker)->GetBelongingId());
			}
			else {
				item.SetBelongingId(attacker->GetObjectID());
			}
			item.SetItemType(Item::GetRandomItemType());
			item.SetPos(GetPos());
		//패킷 전달
   			
			Protocol::S_ITEM pkt;
			Protocol::ItemInfo* iteminfo = pkt.mutable_iteminfo(); //message구성하는 struct pointer반환
			*iteminfo = item.itemInfo;

			GRoom->AddItem(item);

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_Item(pkt);
			GRoom->Broadcast(sendBuf);
		}
	}
		return isalive;
}
