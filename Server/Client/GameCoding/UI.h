#pragma once

class Sprite;

class UI
{
public:
	UI();
	virtual ~UI();

	virtual void BeginPlay();
	virtual void Tick();
	virtual void Render(HDC hdc);

	void SetPos(Vec2 pos) { _pos = pos; }
	Vec2 GetPos() { return _pos; }

	void SetSize(Vec2Int size) { _size = size; }
	Vec2Int GetSize() { return _size; }

	RECT GetRect();
	bool IsMouseInRect();
	void SetCurrentSprite(shared_ptr<Sprite> sprite) { _currentSprite = sprite; }
	
protected:
	Vec2	_pos = {400, 300};
	Vec2Int _size = {150, 150};
	shared_ptr<Sprite> _currentSprite = nullptr;
};

