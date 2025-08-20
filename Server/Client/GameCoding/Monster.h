#pragma once

#include "Creature.h"

class Flipbook;
class Collider;
class BoxCollider;
class Player;

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

private:
	virtual void TickIdle() override;
	virtual void TickMove() override;
	virtual void TickSkill() override;

	virtual void UpdateAnimation() override;

private:
	shared_ptr<Flipbook> _flipbookMove[4] = {};
	float _waitSeconds = 0.f;

	shared_ptr<Player> _target = nullptr; // TEMP
};

