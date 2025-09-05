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
	info.set_defence(0); //���߿� data sheet���� �о���� ���
}

Monster::~Monster()
{

}

void Monster::Init()
{

}

void Monster::Update()//������ ���ν����� �����̶� lock�Ű�X, send�� ������ �ص��Ǳ���
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

//void Monster::UpdateIdle()
//{
//
//	if (!room) return;
//
//	// Ÿ�� ������ ���� Ž��
//	if (_target.lock() == nullptr)
//		_target = room->FindClosestPlayer(GetCellPos());
//	shared_ptr<Player> player = _target.lock();
//	if (!player) return;
//
//	// =============================
//	// 1) ���� ���� ����
//	// =============================
//	Vec2Int d = player->GetCellPos() - GetCellPos();
//	int dist = abs(d.x) + abs(d.y);
//	if (dist <= 1) // �پ������� �ٷ� ����
//	{
//		SetDir(GetLookAtDir(player->GetCellPos()));
//		SetState(SKILL, true);
//		_waitUntil = GetTickCount64() + 1000; // ���� ��ٿ�
//		return;
//	}
//
//	// =============================
//	// 2) ��� Ž�� (��ٿ� ����)
//	// =============================
//	uint64 now = GetTickCount64();
//	if (now > _lastPathUpdate + _pathUpdateInterval)
//	{
//		_path.clear();
//		if (room->MyFindPath(GetCellPos(), player->GetCellPos(), OUT _path))
//		{
//			_lastPathUpdate = now;
//		}
//	}
//
//	// =============================
//	// 3) ��� ���󰡱�
//	// =============================
//	if (_path.size() > 1)
//	{
//		Vec2Int nextPos = _path[1];
//		if (room->CanGo(nextPos))
//		{
//			SetDir(GetLookAtDir(nextPos));
//			SetCellPos(nextPos);
//			SetState(MOVE, true);
//			_waitUntil = now + 200; // �̵� ��
//			_path.erase(_path.begin()); // ù ĭ �Һ�
//		}
//	}
//}


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
			_waitUntil = GetTickCount64() + 500; //+1��
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
						_waitUntil = GetTickCount64() + 500; //+1��
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
	
	//SetState(IDLE,true);
	SetState(IDLE);
}

void Monster::UpdateSkill()
{
	int64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	//SetState(IDLE,true);//����?
	SetState(IDLE);
}