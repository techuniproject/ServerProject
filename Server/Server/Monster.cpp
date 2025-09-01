#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "Player.h"


Monster::Monster()
{
	info.set_name("MonsterName");
	info.set_hp(50);
	info.set_maxhp(50);
	info.set_attack(5);
	info.set_defence(0); //나중엔 data sheet으로 읽어오는 방식
}

Monster::~Monster()
{

}

void Monster::Init()
{

}

void Monster::Update()//어차피 메인스레드 로직이라 lock신경X, send도 메인이 해도되긴함
{
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

void Monster::UpdateIdle()
{
	
	if (room == nullptr)
		return;

	
	// Find Player
	if (_target.lock() == nullptr)
		_target = room->FindClosestPlayer(GetCellPos());
	shared_ptr<Player> player = _target.lock();

	if (player)
	{
		Vec2Int dir = player->GetCellPos() - GetCellPos();
		int32 dist = abs(dir.x) + abs(dir.y);
		if (dist == 1)
		{
			SetDir(GetLookAtDir(player->GetCellPos()));
			SetState(SKILL,true);
			_waitUntil = GetTickCount64() + 1000; //+1초
		}
		else
		{
			vector<Vec2Int> path;
			if (room->FindPath(GetCellPos(), player->GetCellPos(), OUT path))
			{
				if (path.size() > 1)
				{
					Vec2Int nextPos = path[1];
					if (room->CanGo(nextPos))
					{
						SetDir(GetLookAtDir(nextPos));
						SetCellPos(nextPos);
						_waitUntil = GetTickCount64() + 1000; //+1초
						SetState(MOVE,true);
					}
				}
				else
					SetCellPos(path[0]);
			}
		}
	}
}

void Monster::UpdateMove()
{
	uint64 now = GetTickCount64();

	if (_waitUntil > now)
		return;
	
	SetState(IDLE,true);
}

void Monster::UpdateSkill()
{
	int64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	SetState(IDLE,true);
}