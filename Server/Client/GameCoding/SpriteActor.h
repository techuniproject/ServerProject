#pragma once
#include "Actor.h"

class Sprite;

class SpriteActor : public Actor
{
	using Super = Actor;
public:
	SpriteActor();
	virtual ~SpriteActor() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;

	void SetSprite(shared_ptr<Sprite> sprite) { _sprite = sprite; }

	void SetSize(Vec2Int size) { _size = size; }
protected:
	Vec2Int _size{ 50,50 };
	shared_ptr<Sprite> _sprite = nullptr;
};

