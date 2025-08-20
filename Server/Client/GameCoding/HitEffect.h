#pragma once
#include "GameObject.h"

class HitEffect : public GameObject
{
public:
	HitEffect();
	virtual ~HitEffect() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	virtual void UpdateAnimation() override;

protected:
};

