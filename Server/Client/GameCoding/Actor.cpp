#include "pch.h"
#include "Actor.h"
#include "Component.h"

Actor::Actor()
{

}

Actor::~Actor()
{
	/*for (shared_ptr<Component> component : _components)
		SAFE_DELETE(component);*/
}

void Actor::BeginPlay()
{
	for (shared_ptr<Component>& component : _components)
	{
		component->BeginPlay();
	}
}

void Actor::Tick()
{
	for (shared_ptr<Component>& component : _components)
	{
		component->TickComponent();
	}
}

void Actor::Render(HDC hdc)
{
	for (shared_ptr<Component>& component : _components)
	{
		component->Render(hdc);
	}
}

void Actor::AddComponent(shared_ptr<Component> component)
{
	if (component == nullptr)
		return;

	//생성자에서 shared_from_this쓰면 안됨 컨트록 블록과 연결 전임
	component->SetOwner(weak_from_this());
	_components.push_back(component);
}

void Actor::RemoveComponent(shared_ptr<Component> component)
{
	auto findIt = std::find(_components.begin(), _components.end(), component);
	if (findIt == _components.end())
		return;

	_components.erase(findIt);
}

void Actor::OnComponentBeginOverlap(shared_ptr<Collider> collider, shared_ptr<Collider> other)
{

}

void Actor::OnComponentEndOverlap(shared_ptr<Collider> collider, shared_ptr<Collider> other)
{

}
