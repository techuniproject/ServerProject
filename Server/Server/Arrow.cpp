#include "pch.h"
#include "Arrow.h"
#include "GameRoom.h"
#include "Monster.h"
#include "Player.h"
#include "GameSession.h"
#include "Tilemap.h"

Arrow::Arrow()
{
	if(room)
		_tileSize=room->_tilemap.GetTileSize();
}

Arrow::~Arrow()
{

}

void Arrow::Init()
{

}

void Arrow::Update()
{
	Super::Update();
}

void Arrow::UpdateIdle()
{
	if (room == nullptr)
		return;

	Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
	Vec2Int curPos = GetCellPos();
	Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];

	if (CanGoBySector(nextPos))
	{
		SetCellPos(nextPos); //상태바꾸는true기준 스냅샷으로 전달함
		room->InsertAtSector(room->GetSectorPos(nextPos.x, nextPos.y), this);
		if (_ifSpawned) {
			//_ifSpawned = false;
			Protocol::S_AddObject AddedArrow;
			*AddedArrow.add_objects() = info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedArrow);
			room->BroadcastBySector(sendBuf, room->GetSectorPos(info.posx(), info.posy()));
		}
		else {
			if (room->CheckValidSectorPos(room->GetSectorPos(nextPos.x, nextPos.y))) {
				if (!room->IsSameSector(curPos, nextPos)) {
					auto AddMeForNewSectors = [&](Vec2Int sectorPos) {
						Sector* sector = room->GetSectorAt(sectorPos);
						if (sector) {
							for (Player* pl : sector->_sectorPlayers) {
								if (pl) {
									//if (!pl->GetObjectID() == _belongingId) { //내 플레이어도 다른 섹터 갔다왔을때 다시 추가해줘야함
										Protocol::S_AddObject pkt;
										*pkt.add_objects() = info;
										SendBufferRef AddMeToOtherBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
										pl->session->Send(AddMeToOtherBuffer);
									//}
								}
							}
						}
						};
					auto RemoveMeFromLastSectors = [&](Vec2Int sectorPos) {
						Sector* sector = room->GetSectorAt(sectorPos);
						if (sector) {
							for (Player* pl : sector->_sectorPlayers) {
								if (pl) {
									Protocol::S_RemoveObject pkt;
									pkt.add_ids(info.objectid());
									SendBufferRef RemoveMeToOtherBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
									pl->session->Send(RemoveMeToOtherBuffer);
								}
							}
						}
						};
					room->DoSomethingCrossingSectors(nextPos, curPos, AddMeForNewSectors, RemoveMeFromLastSectors);
				}
			}
		}
	
		float moveTimeInSec = (_tileSize)/ info.movespeed();//타일길이를 속도로 나눠 한 타일 이동시간 구한것
		long long moveTick = (long long)(moveTimeInSec * 1000);
		if (_ifSpawned) {
			//클라는 위에서 ifSpawned가 true일때 즉시 addobject패킷 받고 이미 움직이기 시작함.
			//근데 서버는 만약 아래 코드없다면 한칸이동하는 시간을 기다렸다가 플레이어 다음칸으로 이동함
			//이건 즉시 없애줘야 정확한 위치가 맞아떨어짐. 이동도 하기전에 한마디로 이동이 0인데 기다렸다가 가는꼴임
			//첫 타일 이동은 즉시 이뤄져야하므로 waituntil을 기다리지않는 수치로 정한것임.
			_ifSpawned = false;
			_waitUntil = GetTickCount64();
		}
		else {
			_waitUntil = GetTickCount64() + moveTick;
		}
		//SetState(MOVE, true);
		SetState(MOVE);
		//BroadcastMoveBySector();
		//BroadcastMove();
	}
	else
	{
		//여기서 몬스턴지 벽인지 판단해서 피격 추가.
	/*	if (auto creature = room->GetCreatureAtSector(nextPos)) {

			if (dynamic_cast<Monster*>(creature)) {
				if (static_cast<Monster*>(creature)->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this())))
					static_cast<Monster*>(creature)->ApplyHitStun(505);

			}
			else if (dynamic_cast<Player*>(creature)) {
				static_cast<Player*>(creature)->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
					

			}
		}*/
		Monster* mon = room->GetMonsterAtSector(nextPos);
		Player* pl = room->GetPlayerAtSector(nextPos);
		if (mon) {
			//몬스터만 판정 시
			if (mon->OnDamaged(static_pointer_cast<Creature>(shared_from_this()))) {
				//OnDamaged에서 피가 0이면 Leave를 통해 수명 파기하기때문에 검사해야함
				mon->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
			}
		}
		else if (pl) {
			pl->OnDamaged(static_pointer_cast<Creature>(shared_from_this()));
		}
		if (room) {
			Protocol::S_RemoveObject pkt;
			pkt.add_ids(GetObjectID());
			SendBufferRef sendBuf = ServerPacketHandler::Make_S_RemoveObject(pkt);
			GRoom->BroadcastBySector(sendBuf, GRoom->GetSectorPos(info.posx(), info.posy()));
			//GRoom->Broadcast(sendBuf);

			//현 구조에서 Broadcast로 해야 화살이 sector경계간 통신이 끊기지 않고 제거까지 됨
			// Sector기반으로하면 결국 쏜 사람은 화살이 만약 다른 통신범위아닌 섹터까지 가면 생존여부알수없어 클라에선 끊긴 자리에 머무름
			// 하지만, 화살자체는 서버에서 관리하니까 제거되긴함 밑에 list에 넣음으로서
			//근데 Leave되는곳에서 섹터에서 제거할진 또 고려해야함.

			//room->Leave(info.objectid());
			room->AddDeleteProjectiletoList(info.objectid());
		}
		
	}
}

void Arrow::UpdateMove()
{
	uint64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	SetState(IDLE);
}
