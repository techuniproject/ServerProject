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
#include "UI.h"


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

	shared_ptr<UI> curWeaponUI = make_shared<UI>();
	curWeaponUI->SetPos(Vec2(745, 25));
	curWeaponUI->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"Sword"));
	curWeaponUI->SetSize(Vec2Int(50, 50));
	
	GET_SINGLE(GameInstance)->AddUI(L"MyPlayerWeapon", curWeaponUI);

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
		GET_SINGLE(GameInstance)->GetUI(L"MyPlayerWeapon")->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"Sword"));
		SetWeaponType(SWORD);
	}
	else if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::KEY_2))
	{
		GET_SINGLE(GameInstance)->GetUI(L"MyPlayerWeapon")->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"Bow"));
		SetWeaponType(BOW);
	}
	else if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::KEY_3))
	{
		GET_SINGLE(GameInstance)->GetUI(L"MyPlayerWeapon")->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"Staff"));
		SetWeaponType(STAFF);
	}

	if (GET_SINGLE(GameInstance)->GetButtonDown(KeyType::Q)) {
		SetMoveSpeed(GetMoveSpeed() + 100);
	}

	/*if (GET_SINGLE(GameInstance)->GetButton(KeyType::SpaceBar))
	{
		SetState(SKILL);
	}*/

	uint64 now = GetTickCount64();
	bool pressed = GET_SINGLE(GameInstance)->GetButton(KeyType::SpaceBar);
	bool justPressed = pressed && !prevPressed;
	prevPressed = pressed;

	if (pressed&& now >= _nextSkillAt) {
		SetState(SKILL);    
		_nextSkillAt = now + SKILL_CD;    //공격 애니메이션 다 0.5초   
	}
 }
//Myplayer은 지금 클라에서 미리 움직이고, 서버에 통보하고 나머지 클라에 broadcast
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
	
}

void MyPlayer::SyncToServer()
{
	if (_dirtyFlag == false)
		return;
	SendBufferRef sendBuffer2 = ClientPacketHandler::Make_C_Speed(0, GetMoveSpeed());
	GET_SINGLE(GameInstance)->SendPacket(sendBuffer2);
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Move();
	GET_SINGLE(GameInstance)->SendPacket(sendBuffer);
	a = info.state();
	d++;//디버그용
	//벽에 충돌하면 이 함수 호출이 엄청 많아짐 ->원인 찾기
	// 이동 및 스킬 사용시 2번씩 호출됨.
	
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
	if(!GET_SINGLE(GameInstance)->GetButton(KeyType::Shift))
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

void MyPlayer::TickHit()
{
	Super::TickHit();
}
