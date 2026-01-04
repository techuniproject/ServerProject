#pragma once

#include "FlipbookActor.h"
#include "Creature.h"

class Flipbook;
class Collider;
class BoxCollider;

class Player : public Creature
{
	using Super = Creature;
public:
	Player();
	virtual ~Player() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	virtual void AttatchDefaultComponent()override;
protected:
	virtual void TickIdle() override;
	virtual void TickMove() override;
	virtual void TickSkill() override;
	virtual void TickHit() override;

	virtual void UpdateAnimation() override;
public:
	void SetWeaponType(Protocol::WEAPON_TYPE weaponType);
	Protocol::WEAPON_TYPE GetWeaponType() { return info.weapontype(); }
	
	void SetMoveSpeed(float _speed);
	float GetMoveSpeed() { return _moveSpeed; }
private:
	shared_ptr<Flipbook> _flipbookIdle[4] = {};
	shared_ptr<Flipbook> _flipbookMove[4] = {};
	shared_ptr<Flipbook> _flipbookAttack[4] = {};
	shared_ptr<Flipbook> _flipbookBow[4] = {};
	shared_ptr<Flipbook> _flipbookStaff[4] = {};

	float _moveSpeed = 200.f;

	bool _keyPressed = false;
};

