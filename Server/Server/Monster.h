#pragma once
#include "Creature.h"
#include "Item.h"


class Monster :public Creature
{
	using Super = Creature;

public:
	Monster();
	virtual ~Monster() override;

	virtual void Init();
	virtual void Update()override;

	void ApplyHitStun(uint32 ms);

	virtual void OnDamaged(shared_ptr<Creature> attacker)override;
private:
	virtual void UpdateIdle()override;
	virtual void UpdateMove()override;
	virtual void UpdateSkill()override;
	virtual void UpdateHit()override;


private:
	uint64 _waitUntil = 0;

	std::vector<Vec2Int> _path;           // 현재 따라가야 할 경로
	//uint64 _lastPathUpdate;               // 마지막 경로 갱신 시각
	//uint64 _pathUpdateInterval;           // 몇 ms마다 경로 갱신할지

	Item item;

	weak_ptr<Player> _target; // TEMP
};

