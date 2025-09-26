#pragma once
#include "GameObject.h"

enum HITEFFECT {
	SWORD_SNAKE,
	STAFF_SNAKE,
	SNAKE_PLAYER,
};

class HitEffect : public GameObject
{
	using Super = GameObject;

public:
	HitEffect();
	virtual ~HitEffect() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;
	
	virtual void UpdateAnimation() override;

	void SetEffectType(HITEFFECT _effectType) {
		_EffectType = _effectType;
	}
	HITEFFECT GetEffectType() { return _EffectType; }

protected:
	HITEFFECT _EffectType = SNAKE_PLAYER;
};

