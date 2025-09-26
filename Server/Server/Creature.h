#pragma once
#include "GameObject.h"

class Creature:public GameObject
{
	using Super = GameObject;
public:
	Creature();
	virtual ~Creature() override;


	virtual void OnDamaged(shared_ptr<Creature> attacker);
protected:
	virtual void Update();
private:
	
	virtual void UpdateIdle() {};
	virtual void UpdateMove() {};
	virtual void UpdateSkill() {};
	virtual void UpdateHit() {};
};

