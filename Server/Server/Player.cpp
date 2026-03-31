#include "pch.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "Monster.h"
#include "Arrow.h"

Player::Player()
{
	_waitUntil = GetTickCount64();

	// 희박한 버그 방지용 -> 만약 클라가 접속과 동시 스킬을 활로 쏘면,
	// waitUntil이 초기값 0인상태로 활을 쏴 클라 애니메이션 종료 시간 기다리기도 전에
	// 서버에선 화살을 쏜 판정을 하여 클라에 부자연스러운 연출과 판정이 들어감
	// 하지만 그럴수가 없다 거의 
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
	//float moveTimeInSec = 48.0f / info.movespeed();
	//long long moveTick = (long long)(moveTimeInSec * 1000);
	//_waitUntil = GetTickCount64() + moveTick;
	_waitUntil = GetTickCount64() + info.attackspeed() * 850;
	//이건 클라에서 attackspeed가 애니메이션 지속시간이기도 하니까, 물론 스킬 쿨타임과도 연동되어있고,
	//서버에서도 화살을 이 시간 기다렸다가 쏘는게 정상.
}

void Player::UpdateMove()
{
	SetState(IDLE);
	
}

void Player::UpdateSkill()
{
	auto roomref = room.lock();
	if (!roomref) return;

	uint64 now = GetTickCount64();

	//if (now < _stateExitAt) return;

	if (info.weapontype() == Protocol::WEAPON_TYPE_SWORD) {
		if (auto monster = GRoom->GetMonsterAtSector(GetFrontCellPos())) // 전방 셀 타격
		{
			if (monster->OnDamaged(static_pointer_cast<Creature>(shared_from_this()))) {
				//OnDamaged에서 피가 0이면 Leave를 통해 수명 파기하기때문에 검사해야함
				monster->ApplyHitStun(505);
				// 몬스터가 맞고나서 idle로 돌아가는 시간을 505ms로 정하면 플레이어 공격 쿨타임 0.5보다 길어
				// 플레이어가 연달아 공격하는 구조 나옴 -> 하지만 둘다 500ms면 둘다 state가 돌아와서 동시에 때림
				
			}
		}
	}
	else if (info.weapontype() == Protocol::WEAPON_TYPE_BOW)
	{
		if (_waitUntil > now) {
			return;
		}
		shared_ptr<Arrow>arrow = GameObject::CreateArrow();
		arrow->info.set_posx(info.posx());
		arrow->info.set_posy(info.posy());
		arrow->SetInitialPos(Vec2Int(info.posx(), info.posy()));
		arrow->info.set_dir(info.dir());
		arrow->info.set_state(IDLE);
		arrow->info.set_attack(info.attack());
		
		float speed = info.attackspeed();
		speed = 480 + 96 * (1-info.attackspeed()*2);
		//player attackspeed이젠 0.5에서 -> 0.1로 감소 
		arrow->SetBelongingId(info.objectid());
		arrow->info.set_movespeed(speed);
		roomref->Enter(arrow);

		/*Protocol::S_AddObject AddedArrow;
		*AddedArrow.add_objects() = arrow->info;

		SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedArrow);
		gameRoom->BroadcastBySector(sendBuf, gameRoom->GetSectorPos(arrow->info.posx(), arrow->info.posy()));*/
		arrow->SetIfSpawned(true);
	}
	
	SetState(IDLE);
}

void Player::UpdateHit()
{

}
