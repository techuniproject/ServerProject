#pragma once
#include "Creature.h"

class Player : public Creature
{
	using Super = Creature;

public:
	Player();
	virtual ~Player();

public:
	GameSessionRef session; //�ٸ� Ŭ���� ���縦 �˱� ���� ���â�� -���߿� private���� ����
private:
	
};

