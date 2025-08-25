#include "pch.h"
#include "GameObject.h"
#include "Monster.h"
#include "Player.h"
#include "GameRoom.h"

atomic<uint64> GameObject::s_idGenerator = 1;

GameObject::GameObject()
{

}

GameObject::~GameObject()
{

}

shared_ptr<Player> GameObject::CreatePlayer()
{
	shared_ptr<Player> player = make_shared<Player>();
	player->info.set_objectid(s_idGenerator++);
	player->info.set_objecttype(Protocol::OBJECT_TYPE_PLAYER);
	return player;
}


shared_ptr<Monster> GameObject::CreateMonster()
{
	shared_ptr<Monster> monster = make_shared<Monster>();
	monster->info.set_objectid(s_idGenerator++);
	monster->info.set_objecttype(Protocol::OBJECT_TYPE_MONSTER);
	return monster;
}