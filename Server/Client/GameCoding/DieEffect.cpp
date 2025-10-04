#include "pch.h"
#include "DieEffect.h"
#include "GameInstance.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "Scene.h"



DieEffect::DieEffect()
{
	SetLayer(LAYER_EFFECT);
	UpdateAnimation();
}

DieEffect::~DieEffect()
{

}

void DieEffect::BeginPlay()
{
	Super::BeginPlay();
}

void DieEffect::Tick()
{
	Super::Tick();

	if (IsAnimationEnded())
	{
		//Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
		/*Scene* scene = &GET_SINGLE(SceneManager)->GetCurrentScene();
		scene->RemoveActor(this);*/
		GET_SINGLE(GameInstance)->GetCurrentScene().RemoveActor(shared_from_this());
	}
}

void DieEffect::Render(HDC hdc)
{
	Super::Render(hdc);

}

void DieEffect::UpdateAnimation()
{
	SetFlipbook(GET_SINGLE(GameInstance)->GetFlipbook(L"FB_Die"));	
}
