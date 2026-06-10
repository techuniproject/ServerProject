#pragma once
#include "Creature.h"

class Player : public Creature
{
	using Super = Creature;

public:
	Player();
	virtual ~Player();
	//virtual void Init();
	virtual void Update()override;

	void SetCoolTime(uint64 coolTime) { _waitUntil = coolTime; }
	void SetStateHitEndTime(uint64 coolTime) { StateHitEndTime = coolTime; }
	uint64 GetCoolTime() { return _waitUntil; }
	uint64 GetStateHitEndTime() { return StateHitEndTime; }
	bool GetSwordHitDone() { return SwordHitDone; }
	bool GetBowHitDone() { return BowHitDone; }
	void SetBowHitDone(bool done) { BowHitDone = done; }
	void SetSwordHitDone(bool done) { SwordHitDone = done; }
private:
	virtual void UpdateIdle()override;
	virtual void UpdateMove()override;
	virtual void UpdateSkill()override;
	virtual void UpdateHit()override;

	uint64 StateHitEndTime = 0;
	uint64 _waitUntil = 0;
	bool SwordHitDone = false;
	bool BowHitDone = false;
public:
	GameSessionRef session; //다른 클라의 존재를 알기 위한 통신창구 -나중에 private으로 막기

};

