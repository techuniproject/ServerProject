#include "pch.h"
#include "GameInstance.h"
#include "HitEffect.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "Scene.h"

HitEffect::HitEffect()
{
	SetLayer(LAYER_EFFECT);
	UpdateAnimation();
}

HitEffect::~HitEffect()
{

}

void HitEffect::BeginPlay()
{
	__super::BeginPlay();
}

void HitEffect::Tick()
{
	__super::Tick();

	if (IsAnimationEnded())
	{
		//Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
		/*Scene* scene = &GET_SINGLE(SceneManager)->GetCurrentScene();
		scene->RemoveActor(this);*/
		GET_SINGLE(GameInstance)->GetCurrentScene().RemoveActor(shared_from_this());
	}
}

void HitEffect::Render(HDC hdc)
{
	__super::Render(hdc);

}

void HitEffect::UpdateAnimation()
{
	SetFlipbook(GET_SINGLE(GameInstance)->GetFlipbook(L"FB_Hit"));
}
