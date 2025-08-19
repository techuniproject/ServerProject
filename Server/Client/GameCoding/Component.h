#pragma once

class Actor;

class Component : public enable_shared_from_this<Component>
{
public:
	Component();
	virtual ~Component();

	virtual void BeginPlay() {}
	virtual void TickComponent() {}
	virtual void Render(HDC hdc) { }

	void SetOwner(weak_ptr<Actor> owner) { _owner = move(owner); }
	shared_ptr<Actor> GetOwner()const;
	
protected:
	weak_ptr<Actor> _owner;
};

