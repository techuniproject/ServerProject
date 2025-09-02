#pragma once
#include "Creature.h"

class Monster :public Creature
{
	using Super = Creature;

public:
	Monster();
	virtual ~Monster() override;

	virtual void Init();
	virtual void Update()override;


private:
	virtual void UpdateIdle();
	virtual void UpdateMove();
	virtual void UpdateSkill();


private:
	uint64 _waitUntil = 0;

	std::vector<Vec2Int> _path;           // 현재 따라가야 할 경로
	uint64 _lastPathUpdate;               // 마지막 경로 갱신 시각
	uint64 _pathUpdateInterval;           // 몇 ms마다 경로 갱신할지


	weak_ptr<Player> _target; // TEMP
};

