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
	if (_attackRequested)// && now >= _attackReadyAt) {
	{
		_attackRequested = false;
		SetState(SKILL, true);          // 서버가 상태 전이 확정
		//_stateExitAt = now + 150;       // startup 150ms
	}

}

void Player::UpdateMove()
{
	
}

void Player::UpdateSkill()
{
	uint64 now = GetTickCount64();
	//if (now < _stateExitAt) return;

	if (info.weapontype() == Protocol::WEAPON_TYPE_SWORD) {
		if (auto monster = GRoom->GetCreatureAt(GetFrontCellPos())) // 전방 셀 타격
		{
			monster->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
			if (auto m = std::dynamic_pointer_cast<Monster>(monster)) {
				m->ApplyHitStun(500); 
			}
		}
	}
	else if (info.weapontype() == Protocol::WEAPON_TYPE_BOW)
	{

	}
	//_attackReadyAt = now + 350;       // 쿨다운
	SetState(IDLE);
}

void Player::UpdateHit()
{

}