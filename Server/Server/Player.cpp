#include "pch.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Monster.h"

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
 	uint64 now = GetTickCount64();
	if (_attackRequested)
	{
		_attackRequested = false;
		SetState(SKILL, true);         
	}

}

void Player::UpdateMove()
{
	
}

void Player::UpdateSkill()
{
	uint64 now = GetTickCount64();
	//if (now < _stateExitAt) return;
	int a = info.state();
	if (info.weapontype() == Protocol::WEAPON_TYPE_SWORD) {
		if (auto monster = GRoom->GetCreatureAt(GetFrontCellPos())) // 전방 셀 타격
		{
			monster->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
			if (auto m = std::dynamic_pointer_cast<Monster>(monster)) {
				m->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
			}
		}
	}
	else if (info.weapontype() == Protocol::WEAPON_TYPE_BOW)
	{

	}
	
	SetState(IDLE);
}

void Player::UpdateHit()
{

}