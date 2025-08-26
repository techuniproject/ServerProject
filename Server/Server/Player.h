#pragma once
#include "Creature.h"

class Player : public Creature
{
	using Super = Creature;

public:
	Player();
	virtual ~Player();

public:
	GameSessionRef session; //다른 클라의 존재를 알기 위한 통신창구 -나중에 private으로 막기
private:
	
};

