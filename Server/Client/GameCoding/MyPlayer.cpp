#include "pch.h"
#include "GameInstance.h"
#include "MyPlayer.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "Arrow.h"
#include "HitEffect.h"


MyPlayer::MyPlayer()//카메라 달린 플레이어 이동관련 처리
{
}

MyPlayer::~MyPlayer()
{
}

void MyPlayer::AttatchDefaultComponent()
{	
	shared_ptr<CameraComponent> camera = make_shared<CameraComponent>();
	AddComponent(camera);
}

void MyPlayer::TickInput()
{
	
	_keyPressed = true;

	if (GET_SINGLE(GameInstance)->GetButton(KeyType::W))
	{
		SetDir(DIR_UP);
	}
	else  if (GET_SINGLE(GameInstance)->GetButton(KeyType::S))
	{
		SetDir(DIR_DOWN);

	}
	else if (GET_SINGLE(GameInstance)->GetButton(KeyType::A))
	{
		SetDir(DIR_LEFT);
		
	}
	else if (GET_SINGLE(GameInstance)->GetButton(KeyType::D))
	{
		SetDir(DIR_RIGHT);
	
	}
	else
	{
		_keyPressed = false;

	}

	if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::KEY_1))
	{
		SetWeaponType(WeaponType::Sword);
	}
	else if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::KEY_2))
	{
		SetWeaponType(WeaponType::Bow);
	}
	else if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::KEY_3))
	{
		SetWeaponType(WeaponType::Staff);
	}

	if (GET_SINGLE(GameInstance)->GetButton(KeyType::SpaceBar))
	{
		SetState(SKILL);
	}
}

void MyPlayer::TryMove()
{
	if (_keyPressed == false)
		return;
	Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
	Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
	if (CanGo(nextPos))
	{
		SetCellPos(nextPos);
		SetState(MOVE);
	}
	/*if (info.dir()==DIR_UP)
	{
	
		Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(MOVE);
		}
	}
	else  if (info.dir() == DIR_DOWN)
	{
		
		Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(MOVE);
		}
	}
	else if (info.dir() == DIR_LEFT)
	{
		Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(MOVE);
		}
	}
	else if (info.dir() == DIR_RIGHT)
	{

		Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(MOVE);
		}
	}*/
}

void MyPlayer::SyncToServer()
{
	if (_dirtyFlag == false)
		return;

	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Move();
	GET_SINGLE(GameInstance)->SendPacket(sendBuffer);


}


void MyPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void MyPlayer::Tick()
{
	Super::Tick();
	
	//프레임마다 상태 바뀜을 감지하여 서버에 통보 (서버라 매 프레임 보내는게 정석은 아님)
	SyncToServer();
}

void MyPlayer::Render(HDC hdc)
{
	Super::Render(hdc);
}

void MyPlayer::TickIdle()
{
	TickInput();
	TryMove();
}

void MyPlayer::TickMove()
{
	Super::TickMove();
}

void MyPlayer::TickSkill()
{
	Super::TickSkill();
}
