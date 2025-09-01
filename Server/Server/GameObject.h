#pragma once

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Update();

	static shared_ptr<class Player> CreatePlayer();
	static shared_ptr<class Monster> CreateMonster();

public:
	void SetState(ObjectState state, bool broadcast = false);
	void SetDir(Dir dir, bool broadcast=false);
	//bool HasReachedDest();
	bool CanGo(Vec2Int cellPos);
	Dir GetLookAtDir(Vec2Int cellPos);
	void SetCellPos(Vec2Int cellPos, bool broadcast = false);
	Vec2Int GetCellPos();
	Vec2Int GetFrontCellPos();

	void BroadcastMove();

	int64 GetObjectID() { return info.objectid(); }
	void SetObjectID(int64 id) { info.set_objectid(id); }
public:
	Protocol::ObjectInfo info;
	shared_ptr<class GameRoom> room; //순환참조 생길수도 순서 잘 안지키면
	//weak_ptr<class GameRoom>room;
private:
	static atomic<uint64> s_idGenerator;
};


