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
	default:
		break;

	}
}


void Creature::OnDamaged(shared_ptr<Creature>  attacker)
{

	if (attacker == nullptr)
		return;

	int32 damage = attacker->GetObjectAttack() - GetObjectDefence();
	if (damage <= 0)
		return;

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
	}
}