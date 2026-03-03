#pragma once
#include "Player.h"


class MyPlayer : public Player
{
	using Super = Player;
public:
	MyPlayer();
	virtual ~MyPlayer() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;
	virtual void AttatchDefaultComponent()override;
private:
	void TickInput();
	void TryMove();

	void SyncToServer(int& frame);

	virtual void TickIdle() override;
	virtual void TickMove() override;
	virtual void TickSkill() override;
	virtual void TickHit() override;

private:	
	bool _keyPressed = false;
	uint64 _nextSkillAt = 0;          
	uint64 SKILL_CD = 500; 
	bool prevPressed = false;
	bool _isMoving = false;
};

