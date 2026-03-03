#pragma once



struct Item
{
	Item() {
		itemInfo.set_itemid(s_itemidGenerator++);
	};
	~Item() {};

	void SetAliveState(bool alive) { itemInfo.set_isalive(alive); }
	void SetBelongingId(uint32 id) { itemInfo.set_playerid(id); }
	void SetPos(Vec2Int pos) { itemInfo.set_posx(pos.x); itemInfo.set_posy(pos.y); }
	void SetItemType(Protocol::ITEM_TYPE type) { itemInfo.set_itemtype(type); }
	uint32 GetItemID() { return itemInfo.itemid(); }
	Vec2Int GetCellPos() { return  Vec2Int(itemInfo.posx(), itemInfo.posy()); }
	Protocol::ITEM_TYPE GetItemType() { return itemInfo.itemtype(); }
	uint32 GetBelongingID() { return itemInfo.playerid(); }

	static Protocol::ITEM_TYPE GetRandomItemType();
	//bool _isAlive=false;
	//uint32 _belongingid=0;//몬스터 드랍시 처치한 플레이어만 먹을 수 있도록 식별하기 위함.
	//Vec2Int _pos = { 0,0 };
	//Protocol::ITEM_TYPE _itemType= Protocol::ITEM_TYPE_ATTACK;
	Protocol::ItemInfo itemInfo;
private:
	static atomic<uint64> s_itemidGenerator;
};

