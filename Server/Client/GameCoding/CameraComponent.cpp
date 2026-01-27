#include "pch.h"
#include "GameInstance.h"
#include "CameraComponent.h"
#include "Actor.h"
#include "SceneManager.h"

CameraComponent::CameraComponent()
{

}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::BeginPlay()
{

}

void CameraComponent::TickComponent()
{

	Vec2 pos;
	if (shared_ptr<Actor>owner = _owner.lock()) {
		pos = owner->GetPos();
	}
	else {
		return;
	}

	
	// TEMP
	pos.x = ::clamp(pos.x, 400.f, 3024.f - 400.f);
	pos.y = ::clamp(pos.y, 300.f, 2064.f - 300.f);

	GET_SINGLE(GameInstance)->SetCameraPos(pos);

	//Rect는 800x600크기 인데 어차피 pos가 클램프해주고있으니 pos이용
	LONG leftX = (LONG)pos.x - 400;
	LONG topY = (LONG)pos.y - 300;
	GET_SINGLE(GameInstance)->SetCameraRect(RECT{ leftX,topY,leftX + 800,topY + 600 });
}

void CameraComponent::Render(HDC hdc)
{

}
