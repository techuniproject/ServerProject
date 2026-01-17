#include "pch.h"
#include "Arrow.h"
#include "GameRoom.h"
#include "Monster.h"

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

	if (CanGo(nextPos))
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
		if (auto creature = room->GetCreatureAt(nextPos)) {
			creature->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
				if (auto m = std::dynamic_pointer_cast<Monster>(creature)) {
					m->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
				}
			
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
