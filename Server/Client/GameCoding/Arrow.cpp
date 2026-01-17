#include "pch.h"
#include "Arrow.h"
#include "GameInstance.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "Creature.h"
#include "HitEffect.h"

Arrow::Arrow()
{
	_flipbookMove[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_ArrowUp");
	_flipbookMove[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_ArrowDown");
	_flipbookMove[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_ArrowLeft");
	_flipbookMove[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_ArrowRight");

}

Arrow::~Arrow()
{

}

void Arrow::BeginPlay()
{
	Super::BeginPlay();
	UpdateAnimation();
}

void Arrow::Tick()
{
	Super::Tick();
	
	//SyncToServer();
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	switch (info.dir())
	{

	case DIR_UP:
		_pos.y -= info.movespeed() * deltaTime;
		if (_pos.y <= _destPos.y) {
			_pos = _destPos;
			
		}
		break;
	case DIR_DOWN:
		_pos.y += info.movespeed() * deltaTime;
		if (_pos.y >= _destPos.y) {
			_pos = _destPos;
			
		}
		break;
	case DIR_LEFT:
		_pos.x -= info.movespeed() * deltaTime;
		if (_pos.x <= _destPos.x) {
			_pos = _destPos;
			
		}
		break;
	case DIR_RIGHT:
		_pos.x += info.movespeed() * deltaTime;
		if (_pos.x >= _destPos.x) {
			_pos = _destPos;
			
		}
		break;
	}
}

void Arrow::Render(HDC hdc)
{
	Super::Render(hdc);


}

void Arrow::SyncToServer()
{
	/*if (_dirtyFlag == false)
		return;

	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Arrow(belongingId, info.posx(), info.posy(), info.dir(), info.state());
	GET_SINGLE(GameInstance)->SendPacket(sendBuffer);*/

}

void Arrow::TickIdle()
{
	////DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
	//DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
	//if (scene == nullptr)
	//	return;

	//Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
	//Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
	//
	//if (CanGo(nextPos))
	//{
	//	SetCellPos(nextPos);
	//	SetState(MOVE);
	//}
	//else
	//{
	//	//shared_ptr<Creature> creature = scene->GetCreatureAt(nextPos);
	//	//if (creature)
	//	//{
	//	//	scene->SpawnObject<HitEffect>(nextPos);			
	//	//	//creature->OnDamaged(this);
	//	//}

	//	scene->RemoveActor(shared_from_this());
	//}
}

void Arrow::TickMove()
{
	/*float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	switch (info.dir())
	{

	case DIR_UP:
		_pos.y -= 600 * deltaTime;
		if (_pos.y <= _destPos.y) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_DOWN:
		_pos.y += 600 * deltaTime;
		if (_pos.y >= _destPos.y) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_LEFT:
		_pos.x -= 600 * deltaTime;
		if (_pos.x <= _destPos.x) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	case DIR_RIGHT:
		_pos.x += 600 * deltaTime;
		if (_pos.x >= _destPos.x) {
			_pos = _destPos;
			SetState(IDLE);
		}
		break;
	}*/
}

void Arrow::UpdateAnimation()
{
	SetFlipbook(_flipbookMove[info.dir()]);
}
