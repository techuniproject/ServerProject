#pragma once
#include "Creature.h"

class Arrow : public Creature
{
	using Super = Creature;
public:
	Arrow();
	virtual ~Arrow();

	virtual void Init();
	virtual void Update()override;

	void SetBelongingId(uint64 id) { _belongingId = id; }
	uint64 GetBelongingId() { return _belongingId; }

	void SetIfSpawned(bool spawned) { _ifSpawned = spawned; }
private:
	virtual void UpdateIdle()override;
	virtual void UpdateMove()override;
	uint64 _waitUntil = 0;
	uint64 _belongingId = 0;

	bool _ifSpawned = false;
};

