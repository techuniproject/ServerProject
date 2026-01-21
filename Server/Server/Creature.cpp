#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"


Creature::Creature()
{

}

Creature::~Creature()
{

}


void Creature::Update()//어차피 메인스레드 로직이라 lock신경X, send도 메인이 해도되긴함
{
	//여기서 호출하는 로직은 Job으로 처리안해도됨 메인스레드가 함
	//Super::Update();

	switch (info.state())
	{
	case IDLE:
		UpdateIdle();
		break;
	case MOVE:
		UpdateMove();
		break;
	case SKILL:
		UpdateSkill();
		break;
	case HIT:
		UpdateHit();
		break;
	default:
		break;

	}
}


bool Creature::OnDamaged(shared_ptr<Creature>  attacker)
{
	//false면 데미지 applyhitstun도 적용 x ->몬스터의 경우
	// bool로 바꾼건 판정시 Leave를 호출하며 이후에 applyhitstun과 같이 해당 creature정보사용시 null crash 방지
	// 원래 rtti로 동적판정했지만 비용감소
	if (attacker == nullptr)
		return false;

	int32 damage = attacker->GetObjectAttack() - GetObjectDefence();
	if (damage <= 0)
		return false;

	SetObjectHp(max(0, GetObjectHp() - damage));
	if (GetObjectHp() == 0)
	{
		if (GRoom) {

			Protocol::S_RemoveObject pkt;
			pkt.add_ids(GetObjectID());
			SendBufferRef sendBuf = ServerPacketHandler::Make_S_RemoveObject(pkt);
			GRoom->Broadcast(sendBuf);
	
			//여기서 빼버리면 이제 통신은 아예 이 클라와는 못함 해당 플레이어에게 전송안하기때문
			GRoom->Leave(GetObjectID());

			
		}
		return false;
	}
	else {
		if (GRoom) {
			Protocol::S_ATTACK pkt;
			pkt.set_attackedid(GetObjectID());
			pkt.set_hp(info.hp());
			SendBufferRef sendBuf = ServerPacketHandler::Make_S_Attack(pkt);
			GRoom->Broadcast(sendBuf);
		}
		return true;
	}
	
}

