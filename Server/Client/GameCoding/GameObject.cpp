#include "pch.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Creature.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "CameraComponent.h"
#include "DevScene.h"
#include "SceneManager.h"


GameObject::GameObject()
{

}

GameObject::~GameObject()
{

}

void GameObject::BeginPlay()
{
	Super::BeginPlay();

	SetState(MOVE);
	SetState(IDLE);
}

void GameObject::Tick()
{
	_dirtyFlag = false;

	Super::Tick();

	//GET_SINGLE(GameInstance)->GetDeltaTime();
	//Vec2 servPos = Vec2(info.posx(), info.posy());
	//Vec2 diff = servPos - _pos;
	//float dist = diff.Length();
	//float lerpSpeed = 10.f;
	//if (dist > 200.f) {
	//	_pos = servPos;  // 순간이동
	//}
	//else if (dist > 50.f) {
	//	// 빠른 보간
	//	_pos += diff * (GET_SINGLE(GameInstance)->GetDeltaTime() * (lerpSpeed * 3.f));
	//}
	//else {
	//	// 천천히 보간
	//	_pos += diff * (GET_SINGLE(GameInstance)->GetDeltaTime() * lerpSpeed);
	//}


	switch (info.state())
	{
	case IDLE:
		TickIdle();
		break;
	case MOVE:
		TickMove();
		break;
	case SKILL:
		TickSkill();
		break;
	}
}

void GameObject::Render(HDC hdc)
{
	Super::Render(hdc);
}

void GameObject::SetState(ObjectState state)
{
	if (info.state() == state)
		return;

	info.set_state(state);
	//_state = state;
	UpdateAnimation();

	//상태 바뀜
	_dirtyFlag = true;
}

void GameObject::SetDir(Dir dir)
{
	info.set_dir(dir);
	//_dir = dir;
	UpdateAnimation();
	//상태 바뀜
	_dirtyFlag = true;
}

bool GameObject::HasReachedDest()
{
	Vec2 dir = (_destPos - _pos);
	return (dir.Length() < 5.f);
}

bool GameObject::CanGo(Vec2Int cellPos)
{
	//DevScene* scene = dynamic_cast<DevScene*>(GET_SINGLE(SceneManager)->GetCurrentScene());
	DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
	//DevScene* scene = GET_SINGLE(SceneManager)->GetCurrentScene<DevScene>();

	if (scene == nullptr)
		return false;

	return scene->CanGo(cellPos);
}

Dir GameObject::GetLookAtDir(Vec2Int cellPos)
{
	Vec2Int dir = cellPos - GetCellPos();
	if (dir.x > 0)
		return DIR_RIGHT;
	else if (dir.x < 0)
		return DIR_LEFT;
	else if (dir.y > 0)
		return DIR_DOWN;
	else
		return DIR_UP;
}

void GameObject::SetCellPos(Vec2Int cellPos, bool teleport /*= false*/)
{
	info.set_posx(cellPos.x);
	info.set_posy(cellPos.y);
	//GetCellPos() = cellPos;

	//DevScene* scene = dynamic_cast<DevScene*>(GET_SINGLE(SceneManager)->GetCurrentScene());
	DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
	//DevScene* scene = GET_SINGLE(SceneManager)->GetCurrentScene<DevScene>();

	if (scene == nullptr)
		return;

	_destPos = scene->ConvertPos(cellPos);

	if (teleport)
		_pos = _destPos;

	//상태 바뀜
	_dirtyFlag = true;
}

Vec2Int GameObject::GetCellPos()
{
	return Vec2Int(info.posx(), info.posy());
}

Vec2Int GameObject::GetFrontCellPos()
{
	switch (info.dir())
	{
		case DIR_DOWN:
			return GetCellPos() + Vec2Int{0, 1};
		case DIR_LEFT:
			return GetCellPos() + Vec2Int{ -1, 0 };
		case DIR_RIGHT:
			return GetCellPos() + Vec2Int{ 1, 0 };
		case DIR_UP:
			return GetCellPos() + Vec2Int{ 0, -1 };
	}

	return GetCellPos();
}
