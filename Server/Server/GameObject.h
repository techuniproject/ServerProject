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
	shared_ptr<class GameRoom> room; //��ȯ���� ������� ���� �� ����Ű��
	//weak_ptr<class GameRoom>room;
private:
	static atomic<uint64> s_idGenerator;
};


