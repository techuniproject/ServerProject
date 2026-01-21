#include "pch.h"
#include "Arrow.h"
#include "GameRoom.h"
#include "Monster.h"
#include "Player.h"

Arrow::Arrow()
{

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
	Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];

	if (CanGoBySector(nextPos))
	{
		SetCellPos(nextPos); //상태바꾸는true기준 스냅샷으로 전달함
		float moveTimeInSec = 48.0f / info.movespeed();
		long long moveTick = (long long)(moveTimeInSec * 1000);
		_waitUntil = GetTickCount64() + moveTick;
		SetState(MOVE, true);
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
			GRoom->Broadcast(sendBuf);
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
