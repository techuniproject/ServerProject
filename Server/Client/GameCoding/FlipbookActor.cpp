#include "pch.h"
#include "GameInstance.h"
#include "FlipbookActor.h"
#include "Flipbook.h"
#include "Texture.h"
#include "Sprite.h"
#include "TimeManager.h"
#include "SceneManager.h"

FlipbookActor::FlipbookActor()
{
	
}

FlipbookActor::~FlipbookActor()
{

}

void FlipbookActor::BeginPlay()
{
	Super::BeginPlay();
}

void FlipbookActor::Tick()
{
	Super::Tick();

	if (_flipbook == nullptr)
		return;

	const FlipbookInfo& info = _flipbook->GetInfo();
	if (info.loop == false && _idx == info.end)
		return;

	LONG left = (LONG)_pos.x - (LONG)info.size.x;
	LONG top = (LONG)_pos.y - (LONG)info.size.y;
	_rect = RECT{ left,top,left + info.size.x ,top + info.size.y  };

	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	_sumTime += deltaTime*_speed;

	int32 frameCount = (info.end - info.start + 1);
	float delta = info.duration / frameCount;

	if (_sumTime >= delta)
	{
		//delta는 원하는 애니메이션 지속시간을 스프라이트 개수만큼 나눈값을 경계지점
		//그 경계 판단 기준은 sumtime인데 이건 speed를 dt에 곱하니까 speed를 늘리면 빠르게 가능
		//speed로 제어가능한 영역이 어느순간 의미없어짐 무조건 delta보다 커지니까

		_sumTime = 0.f;
		_idx = (_idx + 1) % frameCount;
	}
}

void FlipbookActor::Render(HDC hdc)
{
	Super::Render(hdc);

	if (_flipbook == nullptr)
		return;

	const FlipbookInfo& info = _flipbook->GetInfo();
	Vec2 cameraPos = GET_SINGLE(GameInstance)->GetCameraPos();
	
		::TransparentBlt(hdc,
			(int32)_pos.x - info.size.x / 2 - ((int32)cameraPos.x - GWinSizeX / 2),
			(int32)_pos.y - info.size.y / 2 - ((int32)cameraPos.y - GWinSizeY / 2),
			info.size.x,
			info.size.y,
			info.texture->GetDC(),
			(info.start + _idx) * info.size.x,
			info.line * info.size.y,
			info.size.x,
			info.size.y,
			info.texture->GetTransparent());
	
}

void FlipbookActor::SetFlipbook(shared_ptr<Flipbook> flipbook)
{
	if (flipbook && _flipbook == flipbook)
		return;

	_flipbook = flipbook;
	const FlipbookInfo& info = _flipbook->GetInfo();
	LONG left = _pos.x - info.size.x;
	LONG top = _pos.y - info.size.y;
	_rect = RECT{ left,top,left + info.size.x,top + info.size.y };
	Reset();
}

void FlipbookActor::Reset()
{
	_sumTime = 0.f;
	_idx = 0;
}

bool FlipbookActor::IntersectsWithCamRect(RECT CamRect)
{
	
	return Utils::IsRectIntersect(_rect, CamRect);
}

bool FlipbookActor::IsAnimationEnded()
{
	if (_flipbook == nullptr)
		return true;

	const FlipbookInfo& info = _flipbook->GetInfo();
	if (info.loop == false && _idx == info.end-info.start)
		return true;

	return false;
}
