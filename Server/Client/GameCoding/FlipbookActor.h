#pragma once
#include "Actor.h"

class Flipbook;

class FlipbookActor : public Actor
{
	using Super = Actor;
public:
	FlipbookActor();
	virtual ~FlipbookActor() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	void SetFlipbook(shared_ptr<Flipbook> flipbook);
	void Reset();
	void SetFlipbookSpeed(float speed) { _speed = speed; }
	float GetFlipbookSpeed() { return _speed; }

	RECT GetRect() { return _rect; }

	virtual bool IntersectsWithCamRect(RECT CamRect)override;

	bool IsAnimationEnded();

protected:
	shared_ptr<Flipbook> _flipbook = nullptr;
	float _sumTime = 0.f;
	int32 _idx = 0;
	float _speed = 1.f;
	RECT _rect;
};

