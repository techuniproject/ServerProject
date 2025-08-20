#pragma once
#include "UI.h"

class Sprite;

class Panel : public UI
{
public:
	Panel();
	virtual ~Panel();

	virtual void BeginPlay();
	virtual void Tick();
	virtual void Render(HDC hdc);

	void		AddChild(shared_ptr<UI> ui);
	bool		RemoveChild(shared_ptr<UI> ui);

private:
	vector<shared_ptr<UI>> _children;
};

