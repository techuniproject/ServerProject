#pragma once

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	GameObject();
	virtual ~GameObject();

	static shared_ptr<class Player> CreatePlayer();
	static shared_ptr<class Monster> CreateMonster();
public:
	Protocol::ObjectInfo info;
	shared_ptr<class GameRoom> room; //순환참조 생길수도 순서 잘 안지키면
	//weak_ptr<class GameRoom>room;
private:
	static atomic<uint64> s_idGenerator;
};


