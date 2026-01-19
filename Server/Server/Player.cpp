#include "pch.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Monster.h"
#include "Arrow.h"

Player::Player()
{

}

Player::~Player()
{

}

void Player::Update()//어차피 메인스레드 로직이라 lock신경X, send도 메인이 해도되긴함
{
	
	//여기서 호출하는 로직은 Job으로 처리안해도됨 메인스레드가 함
	Super::Update();

}



void Player::UpdateIdle()
{
	float moveTimeInSec = 48.0f / info.movespeed();
	long long moveTick = (long long)(moveTimeInSec * 1000);
	_waitUntil = GetTickCount64() + moveTick;
}

void Player::UpdateMove()
{
	SetState(IDLE);
	
}

void Player::UpdateSkill()
{
	if (room == nullptr)return;

	uint64 now = GetTickCount64();

	
	//if (now < _stateExitAt) return;
	int a = info.state();
	if (info.weapontype() == Protocol::WEAPON_TYPE_SWORD) {
		if (auto monster = GRoom->GetMonsterAtSector(GetFrontCellPos())) // 전방 셀 타격
		{
			monster->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
			monster->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
			
		}
	}
	else if (info.weapontype() == Protocol::WEAPON_TYPE_BOW)
	{
		if (_waitUntil > now)
			return;
		shared_ptr<GameRoom>gameRoom = room;
		
		shared_ptr<Arrow>arrow = GameObject::CreateArrow();
		arrow->info.set_posx(info.posx());
		arrow->info.set_posy(info.posy());
		arrow->info.set_dir(info.dir());
		arrow->info.set_attack(info.attack());
		float speed = info.attackspeed();
		speed *= 600;
		arrow->SetBelongingId(info.objectid());
		arrow->info.set_movespeed(speed);
		gameRoom->Enter(arrow);
		Protocol::S_AddObject AddedArrow;
		*AddedArrow.add_objects() = arrow->info;

		SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedArrow);
		gameRoom->Broadcast(sendBuf);
	
	}
	
	SetState(IDLE);
}

void Player::UpdateHit()
{

}