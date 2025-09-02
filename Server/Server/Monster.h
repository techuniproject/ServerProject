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

	std::vector<Vec2Int> _path;           // ���� ���󰡾� �� ���
	uint64 _lastPathUpdate;               // ������ ��� ���� �ð�
	uint64 _pathUpdateInterval;           // �� ms���� ��� ��������


	weak_ptr<Player> _target; // TEMP
};

