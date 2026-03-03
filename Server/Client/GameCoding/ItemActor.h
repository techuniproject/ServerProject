#pragma once
#include "Actor.h"

class Sprite;

class ItemActor : public Actor
{
	using Super = Actor;
public:
	ItemActor();
	virtual ~ItemActor() override;

	virtual void BeginPlay()override;
	virtual void Tick()override;
	virtual void Render(HDC hdc)override;
	
	void SetAliveState(bool alive) { itemInfo.set_isalive(alive); }
	void SetBelongingId(uint32 id) { itemInfo.set_playerid(id); }
	void SetCellPos(Vec2Int pos) { itemInfo.set_posx(pos.x); itemInfo.set_posy(pos.y); }
	void SetItemType(Protocol::ITEM_TYPE type) { itemInfo.set_itemtype(type); }
	void SetItemId(uint32 id) { itemInfo.set_itemid(id); }

	uint32 GetItemID() { return itemInfo.itemid(); }
	Vec2Int GetCellPos() { return  Vec2Int(itemInfo.posx(), itemInfo.posy()); }
	Protocol::ITEM_TYPE GetItemType() { return itemInfo.itemtype(); }
	uint32 GetBelongingID() { return itemInfo.playerid(); }

	void SetSprite(shared_ptr<Sprite> sprite) { _sprite = sprite; }

	void SetSize(Vec2Int size) { _size = size; }
private:
	Protocol::ItemInfo itemInfo;
protected:
	Vec2Int _size{ 50,50 };
	shared_ptr<Sprite> _sprite = nullptr;
};

