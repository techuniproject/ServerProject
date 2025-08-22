#pragma once
#include "FlipbookActor.h"

class GameObject : public FlipbookActor
{
	using Super = FlipbookActor;

public:
	GameObject();
	virtual ~GameObject() override;

	virtual void BeginPlay() override;
	virtual void Tick() override;
	virtual void Render(HDC hdc) override;
	
	virtual void AttatchDefaultComponent() {};

	virtual void TickIdle() {}
	virtual void TickMove() {}
	virtual void TickSkill() {}

	void SetState(ObjectState state);
	void SetDir(Dir dir);

	virtual void UpdateAnimation() {}

	bool HasReachedDest();
	bool CanGo(Vec2Int cellPos);
	Dir GetLookAtDir(Vec2Int cellPos);

	void SetCellPos(Vec2Int cellPos, bool teleport = false);
	Vec2Int GetCellPos();
	Vec2Int GetFrontCellPos();

	int64 GetObjectID() { return info.objectid(); }
	void SetObjectID(int64 id) { info.set_objectid(id); }

protected:
	//int64 _objectID = 0;
	//Vec2Int GetCellPos() = {};
	//Vec2 _speed = {};
	//Dir _dir = DIR_DOWN;
	//ObjectState _state = IDLE;
public://나중엔 private관리
	Protocol::ObjectInfo info;
};

