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
	//AddComponent(camera);//여기서 shared_from_this 호출되어 문제
	
	
	_stat.attack = 100;
}

Player::~Player()
{

}

void Player::AttatchDefaultComponent()
{	/*
	shared_from_this()는 이미 이 객체를 소유하고 있는 shared_ptr이 존재한다는 전제 하에서만 동작합니다.
	그런데 make_shared<T>()로 객체를 만들면:
	T의 생성자가 먼저 호출됨
	생성자가 끝난 뒤에야, 그 객체를 소유하는 shared_ptr<T>가 완성됨
	즉, 생성자 실행 중에는 아직 shared_ptr이 객체를 소유하지 않음 → shared_from_this()는 내부적으로 약한 참조(weak_ptr)를 찾는데, 등록된 게 없으니 bad_weak_ptr 예외를 던집니다.
	*/
	shared_ptr<CameraComponent> camera = make_shared<CameraComponent>();
	AddComponent(camera);
}


void Player::BeginPlay()
{
	Super::BeginPlay();

	SetState(ObjectState::Move);
	SetState(ObjectState::Idle);

	SetCellPos({5, 5}, true);
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
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	_keyPressed = true;
	Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

	if (GET_SINGLE(GameInstance)->GetButton(KeyType::W))
	{
		SetDir(DIR_UP);

		Vec2Int nextPos = _cellPos + deltaXY[_dir];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(ObjectState::Move);
		}
	}
	else  if (GET_SINGLE(GameInstance)->GetButton(KeyType::S))
	{
		SetDir(DIR_DOWN);

		Vec2Int nextPos = _cellPos + deltaXY[_dir];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(ObjectState::Move);
		}
	}
	else if (GET_SINGLE(GameInstance)->GetButton(KeyType::A))
	{
		SetDir(DIR_LEFT);
		Vec2Int nextPos = _cellPos + deltaXY[_dir];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(ObjectState::Move);
		}
	}
	else if (GET_SINGLE(GameInstance)->GetButton(KeyType::D))
	{
		SetDir(DIR_RIGHT);
		Vec2Int nextPos = _cellPos + deltaXY[_dir];
		if (CanGo(nextPos))
		{
			SetCellPos(nextPos);
			SetState(ObjectState::Move);
		}
	}
	else
	{
		_keyPressed = false;
		if (_state == ObjectState::Idle)
			UpdateAnimation();
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
		SetState(ObjectState::Skill);
	}
}

void Player::TickMove()
{
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	Vec2 dir = (_destPos - _pos);	
	if (dir.Length() < 5.f)
	{
		SetState(ObjectState::Idle);
		_pos = _destPos;
	}
	else
	{
		switch (_dir)
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
			shared_ptr<Arrow> arrow = scene->SpawnObject<Arrow>(_cellPos);
			arrow->SetDir(_dir);	
		}

		SetState(ObjectState::Idle);
	}
}

void Player::UpdateAnimation()
{
	switch (_state)
	{
	case ObjectState::Idle:
		if (_keyPressed)
			SetFlipbook(_flipbookMove[_dir]); 
		else
			SetFlipbook(_flipbookIdle[_dir]);
		break;
	case ObjectState::Move:
		SetFlipbook(_flipbookMove[_dir]);
		break;
	case ObjectState::Skill:
		if (_weaponType == WeaponType::Sword)
			SetFlipbook(_flipbookAttack[_dir]);
		else if (_weaponType == WeaponType::Bow)
			SetFlipbook(_flipbookBow[_dir]);
		else
			SetFlipbook(_flipbookStaff[_dir]);
		break;
	}
}