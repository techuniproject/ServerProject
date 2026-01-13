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
	virtual void TickHit(){}

	void SetState(ObjectState state);
	void SetDir(Dir dir);

	virtual void UpdateAnimation() {}

	bool HasReachedDest();
	bool CanGo(Vec2Int cellPos);
	Dir GetLookAtDir(Vec2Int cellPos);

	void SetCellPos(Vec2Int cellPos,bool dirtyFlag=true, bool teleport = false);
	Vec2Int GetCellPos();
	Vec2Int GetFrontCellPos();

	int64 GetObjectID() { return info.objectid(); }
	void SetObjectID(int64 id) { info.set_objectid(id); }

	void SetMaxHp(int32 maxHp) { info.set_maxhp(maxHp); }
	void SetHp(int32 hp) { info.set_hp(hp); }
	void SetDefence(int32 defence) { info.set_defence(defence); }
	void SetName(string name) { info.set_name(name); }


	void SetMoveSpeed(float speed, bool dirtyFlag = true) { info.set_movespeed(speed); _dirtyFlag = dirtyFlag; }
	float GetMoveSpeed() { return info.movespeed(); }
	void SetAttackSpeed(float speed,bool dirtyFlag=true) { info.set_attackspeed(speed); _dirtyFlag = dirtyFlag;}
	float GetAttackSpeed() { return info.attackspeed(); }

protected:
	//int64 _objectID = 0;
	//Vec2Int GetCellPos() = {};
	//Vec2 _speed = {};
	//Dir _dir = DIR_DOWN;
	//ObjectState _state = IDLE;
protected://상태 바뀜 추적용 
	bool _dirtyFlag = false;
	
public://나중엔 private관리
	Protocol::ObjectInfo info;
};

