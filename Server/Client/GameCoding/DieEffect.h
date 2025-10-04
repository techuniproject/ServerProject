#pragma once
#include "GameObject.h"


class DieEffect : public GameObject
{
	using Super = GameObject;

public:
	DieEffect();
	virtual ~DieEffect() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	virtual void UpdateAnimation() override;
};

