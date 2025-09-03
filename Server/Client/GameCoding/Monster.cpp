#include "pch.h"
#include "GameInstance.h"
#include "Monster.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "Player.h"
#include "HitEffect.h"


Monster::Monster()
{
	_flipbookMove[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_SnakeUp");
	_flipbookMove[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_SnakeDown");
	_flipbookMove[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_SnakeLeft");
	_flipbookMove[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_SnakeRight");

}

Monster::~Monster()
{

}

void Monster::BeginPlay()
{
	Super::BeginPlay();

	SetState(MOVE);
	SetState(IDLE);
}

void Monster::Tick()
{
	Super::Tick();

}

void Monster::Render(HDC hdc)
{
	Super::Render(hdc);

}

void Monster::TickIdle()
{
	////DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
	//DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
	//
	//if (scene == nullptr)
	//	return;

	//// Find Player
	//if (_target == nullptr)
	//	_target = scene->FindClosestPlayer(GetCellPos());

	//if (_target)
	//{
	//	Vec2Int dir = _target->GetCellPos() - GetCellPos();
	//	int32 dist = abs(dir.x) + abs(dir.y);
	//	if (dist == 1)
	//	{
	//		SetDir(GetLookAtDir(_target->GetCellPos()));
	//		SetState(SKILL);
	//		_waitSeconds = 0.5f; // 공격 종료 시간
	//	}
	//	else
	//	{
	//		vector<Vec2Int> path;
	//		if (scene->FindPath(GetCellPos(), _target->GetCellPos(), OUT path))
	//		{
	//			if (path.size() > 1)
	//			{
	//				Vec2Int nextPos = path[1];
	//				if (scene->CanGo(nextPos))
	//				{
	//					SetCellPos(nextPos);
	//					SetState(MOVE);
	//				}
	//			}
	//			else
	//				SetCellPos(path[0]);
	//		}
	//	}
	//}
}

void Monster::TickMove()
{
	/*float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	Vec2 dir = (_destPos - _pos);
	if (dir.Length() < 5.f)
	{
		SetState(IDLE);
		_pos = _destPos;
	}
	else
	{
		bool horizontal = abs(dir.x) > abs(dir.y);
		if (horizontal)
			SetDir(dir.x < 0 ? DIR_LEFT : DIR_RIGHT);
		else
			SetDir(dir.y < 0 ? DIR_UP : DIR_DOWN);

		switch (info.dir())
		{
		case DIR_UP:
			_pos.y -= 150 * deltaTime;
			break;
		case DIR_DOWN:
			_pos.y += 150 * deltaTime;
			break;
		case DIR_LEFT:
			_pos.x -= 150 * deltaTime;
			break;
		case DIR_RIGHT:
			_pos.x += 150 * deltaTime;
			break;
		}
	}*/

	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();
	deltaTime = min(deltaTime, 0.05f); //프레임드랍 심해져서 0.05보다 커지면 보정
	Vec2 dir = (_destPos - _pos);
	bool horizontal = abs(dir.x) > abs(dir.y);
	if (horizontal)
		SetDir(dir.x < 0 ? DIR_LEFT : DIR_RIGHT);
	else
		SetDir(dir.y < 0 ? DIR_UP : DIR_DOWN);
	switch (info.dir())
	{
		
	case DIR_UP:
		_pos.y -= 150 * deltaTime;
		if (_pos.y <= _destPos.y) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_DOWN:
		_pos.y += 150 * deltaTime;
		if (_pos.y >= _destPos.y) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_LEFT:
		_pos.x -= 150 * deltaTime;
		if (_pos.x <= _destPos.x) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_RIGHT:
		_pos.x += 150 * deltaTime;
		if (_pos.x >= _destPos.x) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	}
}

void Monster::TickSkill()
{
	if (_flipbook == nullptr)
		return;

	if (_waitSeconds > 0)
	{
		float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();
		_waitSeconds = max(0, _waitSeconds - deltaTime);	
		return;
	}

	{
	//	DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
		DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
		if (scene == nullptr)
			return;

		shared_ptr<Creature> creature = scene->GetCreatureAt(GetFrontCellPos());
		if (creature)
		{
			scene->SpawnObject<HitEffect>(GetFrontCellPos());
			creature->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
		}

		SetState(IDLE);
	}
}

void Monster::UpdateAnimation()
{
	SetFlipbook(_flipbookMove[info.dir()]);
}
