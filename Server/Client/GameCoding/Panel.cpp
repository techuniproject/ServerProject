#include "pch.h"
#include "Panel.h"

Panel::Panel()
{

}

Panel::~Panel()
{
	/*for (shared_ptr<UI>& child : _children)
		SAFE_DELETE(child);*/

	_children.clear();
}

void Panel::BeginPlay()
{
	__super::BeginPlay();

	for (shared_ptr<UI>& child : _children)
		child->BeginPlay();
}

void Panel::Tick()
{
	__super::Tick();

	for (shared_ptr<UI>& child : _children)
		child->Tick();
}

void Panel::Render(HDC hdc)
{
	__super::Render(hdc);

	for (shared_ptr<UI>& child : _children)
		child->Render(hdc);
}

void Panel::AddChild(shared_ptr<UI> ui)
{
	if (ui == nullptr)
		return;

	_children.push_back(ui);
}

bool Panel::RemoveChild(shared_ptr<UI> ui)
{
	auto findIt = std::find(_children.begin(), _children.end(), ui);
	if (findIt == _children.end())
		return false;

	// TODO: ªË¡¶?
	_children.erase(findIt);
	return true;
}