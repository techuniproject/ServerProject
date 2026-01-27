#pragma once

class Component;
class Collider;

class Actor : public enable_shared_from_this<Actor>
{
public:
	Actor();
	virtual ~Actor();

	virtual void BeginPlay();
	virtual void Tick();
	virtual void Render(HDC hdc);

	void SetPos(Vec2 pos) { _pos = pos; }
	Vec2 GetPos() { return _pos; }

	void SetLayer(LAYER_TYPE layer) { _layer = layer; }
	LAYER_TYPE GetLayer() { return _layer; }

	void AddComponent(shared_ptr<Component> component);
	void RemoveComponent(shared_ptr<Component> component);

	// OnCollisionEnter2D / OnCollisionExit2D
	virtual void OnComponentBeginOverlap(shared_ptr<Collider> collider, shared_ptr<Collider> other);
	virtual void OnComponentEndOverlap(shared_ptr<Collider> collider, shared_ptr<Collider> other);

	virtual bool IntersectsWithCamRect(RECT CamRect) { return true; }
	//일단 actor은 다 통과되게 -> flipbookactor만 따로 로직수행되도록
	//scene이 actor기반 관리하므로

protected:
	Vec2 _pos = {0, 0};
	Vec2 _destPos = {0, 0};
	LAYER_TYPE _layer = LAYER_OBJECT;
	vector<shared_ptr<Component>> _components;
};

