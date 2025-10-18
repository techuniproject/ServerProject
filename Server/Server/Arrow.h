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

private:
	virtual void UpdateIdle()override;
	virtual void UpdateMove()override;

};

