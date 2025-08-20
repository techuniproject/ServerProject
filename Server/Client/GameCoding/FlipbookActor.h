#pragma once
#include "Actor.h"

class Flipbook;

class FlipbookActor : public Actor
{
public:
	FlipbookActor();
	virtual ~FlipbookActor() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	void SetFlipbook(shared_ptr<Flipbook> flipbook);
	void Reset();

	bool IsAnimationEnded();

protected:
	shared_ptr<Flipbook> _flipbook = nullptr;
	float _sumTime = 0.f;
	int32 _idx = 0;
};

