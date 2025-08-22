#include "pch.h"
#include "GameInstance.h"
#include "Player.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "Arrow.h"
#include "HitEffect.h"


Player::Player()
{
	_flipbookIdle[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_IdleUp");
	_flipbookIdle[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_IdleDown");
	_flipbookIdle[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_IdleLeft");
	_flipbookIdle[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_IdleRight");
	
	_flipbookMove[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_MoveUp");
	_flipbookMove[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_MoveDown");
	_flipbookMove[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_MoveLeft");
	_flipbookMove[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_MoveRight");

	_flipbookAttack[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_AttackUp");
	_flipbookAttack[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_AttackDown");
	_flipbookAttack[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_AttackLeft");
	_flipbookAttack[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_AttackRight");

	_flipbookBow[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_BowUp");
	_flipbookBow[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_BowDown");
	_flipbookBow[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_BowLeft");
	_flipbookBow[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_BowRight");

	_flipbookStaff[DIR_UP] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_StaffUp");
	_flipbookStaff[DIR_DOWN] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_StaffDown");
	_flipbookStaff[DIR_LEFT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_StaffLeft");
	_flipbookStaff[DIR_RIGHT] = GET_SINGLE(GameInstance)->GetFlipbook(L"FB_StaffRight");

	//shared_ptr<CameraComponent> camera = make_shared<CameraComponent>();
	//AddComponent(camera);//���⼭ shared_from_this ȣ��Ǿ� ����
	
	
	_stat.attack = 100;
}

Player::~Player()
{

}

void Player::AttatchDefaultComponent()
{	/*
	shared_from_this()�� �̹� �� ��ü�� �����ϰ� �ִ� shared_ptr�� �����Ѵٴ� ���� �Ͽ����� �����մϴ�.
	�׷��� make_shared<T>()�� ��ü�� �����:
	T�� �����ڰ� ���� ȣ���
	�����ڰ� ���� �ڿ���, �� ��ü�� �����ϴ� shared_ptr<T>�� �ϼ���
	��, ������ ���� �߿��� ���� shared_ptr�� ��ü�� �������� ���� �� shared_from_this()�� ���������� ���� ����(weak_ptr)�� ã�µ�, ��ϵ� �� ������ bad_weak_ptr ���ܸ� �����ϴ�.
	*/
	//shared_ptr<CameraComponent> camera = make_shared<CameraComponent>();
	//AddComponent(camera);
}


void Player::BeginPlay()
{
	Super::BeginPlay();

	SetState(MOVE);
	SetState(IDLE);

	
}

void Player::Tick()
{
	Super::Tick();
}

void Player::Render(HDC hdc)
{
	Super::Render(hdc);

}


void Player::TickIdle()
{
	
}

void Player::TickMove()
{
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	Vec2 dir = (_destPos - _pos);	
	if (dir.Length() < 1.f)
	{
		SetState(IDLE);
		_pos = _destPos;
	}
	else
	{
		switch (info.dir())
		{
		case DIR_UP:
			_pos.y -= 200 * deltaTime;
			break;
		case DIR_DOWN:
			_pos.y += 200 * deltaTime;
			break;
		case DIR_LEFT:
			_pos.x -= 200 * deltaTime;
			break;
		case DIR_RIGHT:
			_pos.x += 200 * deltaTime;
			break;
		}
	}	
}

void Player::TickSkill()
{
	if (_flipbook == nullptr)
		return;

	// TODO : Damage?
	if (IsAnimationEnded())
	{
		//DevScene* scene = dynamic_cast<DevScene*>(&GET_SINGLE(GameInstance)->GetCurrentScene());
		DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
		
		if (scene == nullptr)
			return;

		if (_weaponType == WeaponType::Sword)
		{
			shared_ptr<Creature> creature = scene->GetCreatureAt(GetFrontCellPos());
			if (creature)
			{
				scene->SpawnObject<HitEffect>(GetFrontCellPos());
				creature->OnDamaged(dynamic_pointer_cast<Creature>(shared_from_this()));
			}
		}
		else if (_weaponType == WeaponType::Bow)
		{
			shared_ptr<Arrow> arrow = scene->SpawnObject<Arrow>(GetCellPos());
			arrow->SetDir(info.dir());	
		}

		SetState(IDLE);
	}
}

void Player::UpdateAnimation()
{
	switch (info.state())
	{
	case IDLE:
		/*if (_keyPressed)
			SetFlipbook(_flipbookMove[info.dir()]);
		else*/
			SetFlipbook(_flipbookIdle[info.dir()]);
		break;
	case MOVE:
		SetFlipbook(_flipbookMove[info.dir()]);
		break;
	case SKILL:
		if (_weaponType == WeaponType::Sword)
			SetFlipbook(_flipbookAttack[info.dir()]);
		else if (_weaponType == WeaponType::Bow)
			SetFlipbook(_flipbookBow[info.dir()]);
		else
			SetFlipbook(_flipbookStaff[info.dir()]);
		break;
	}
}