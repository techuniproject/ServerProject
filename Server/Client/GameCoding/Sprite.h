#pragma once
#include "ResourceBase.h"

class Texture;

class Sprite : public ResourceBase
{
public:
	Sprite(shared_ptr<Texture> texture, int32 x, int32 y, int32 cx, int32 cy);
	virtual ~Sprite();

	HDC		GetDC();
	int32	GetTransparent();
	Vec2Int GetPos() { return Vec2Int{ _x, _y }; }
	Vec2Int GetSize() { return Vec2Int{ _cx, _cy }; }

private:
	shared_ptr<Texture> _texture = nullptr;
	int32 _x;
	int32 _y;
	int32 _cx;
	int32 _cy;
};

