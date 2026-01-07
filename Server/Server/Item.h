#pragma once



struct Item
{
	Item() {};
	Item(Protocol::ITEM_TYPE type, Vec2Int pos, uint32 id, uint32 alive = false) :_itemType{ type }, _pos{ pos }, _belongingid{ id },_isAlive(alive) {}
	~Item() {};

	void SetAliveState(bool alive) { _isAlive = alive; }
	void SetBelongingId(uint32 id) { _belongingid = id; }
	void SetPos(Vec2Int pos) { _pos = pos; }
	void SetItemType(Protocol::ITEM_TYPE type) { _itemType = type; }

	static Protocol::ITEM_TYPE GetRandomItemType();
	bool _isAlive=false;
	uint32 _belongingid=0;//몬스터 드랍시 처치한 플레이어만 먹을 수 있도록 식별하기 위함.
	Vec2Int _pos = { 0,0 };
	Protocol::ITEM_TYPE _itemType= Protocol::ITEM_TYPE_ATTACK;

};

