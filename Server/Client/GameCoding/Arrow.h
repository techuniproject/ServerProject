#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{

public:
	Arrow();
	virtual ~Arrow() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	virtual void TickIdle() override;
	virtual void TickMove() override;

	virtual void UpdateAnimation() override;

protected:
	shared_ptr<Flipbook> _flipbookMove[4] = {};
};

