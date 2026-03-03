#include "pch.h"
#include "ItemActor.h"
#include "GameInstance.h"
#include "InputManager.h"
#include "Sprite.h"

ItemActor::ItemActor()
{
	SetLayer(LAYER_ITEM);
}

ItemActor::~ItemActor()
{

}

void ItemActor::BeginPlay()
{
	Super::BeginPlay();
}

void ItemActor::Tick()
{
	Super::Tick();
}

void ItemActor::Render(HDC hdc)
{
	Super::Render(hdc);

	if (_sprite == nullptr)
		return;

	//Vec2Int size = _sprite->GetSize();
	Vec2 cameraPos = GET_SINGLE(GameInstance)->GetCameraPos();

	int32 leftX = ((int32)cameraPos.x - GWinSizeX / 2);
	int32 leftY = ((int32)cameraPos.y - GWinSizeY / 2);
	int32 rightX = ((int32)cameraPos.x + GWinSizeX / 2);
	int32 rightY = ((int32)cameraPos.y + GWinSizeY / 2);

	int32 startX = (leftX - itemInfo.posx()) / 48;
	int32 startY = (leftY - itemInfo.posy()) / 48;
	int32 endX = (rightX - itemInfo.posx()) / 48;
	int32 endY = (rightY - itemInfo.posy()) / 48;
	
	if (itemInfo.posx() <= endX && itemInfo.posx() >= startX && itemInfo.posy() <= endY && itemInfo.posy() >= startY) {
		::TransparentBlt(hdc,
			itemInfo.posx() * _size.x - ((int32)cameraPos.x - GWinSizeX / 2),
			itemInfo.posy() * _size.y - ((int32)cameraPos.y - GWinSizeY / 2),
			_size.x,
			_size.y,
			_sprite->GetDC(),
			_sprite->GetPos().x,
			_sprite->GetPos().y,
			_size.x,
			_size.y,
			_sprite->GetTransparent());
	}
	
}


