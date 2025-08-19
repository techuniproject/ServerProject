#include "pch.h"
#include "Scene.h"
#include "DevScene.h"
#include "EditScene.h"
#include "SceneManager.h"


DEFINE_DEFAULT_DESTRUCTOR(SceneManager)
DEFINE_DEFAULT_CONSTRUCTOR(SceneManager)


void SceneManager::Init()
{

}

void SceneManager::Update()
{
	if (_scene)
		_scene->Update();		
}

void SceneManager::Render(HDC hdc)
{
	if (_scene)
		_scene->Render(hdc);
}

void SceneManager::Clear()
{
	//SAFE_DELETE(_scene);
	_scene.reset();
}

void SceneManager::ChangeScene(SceneType sceneType)
{
	if (_sceneType == sceneType)
		return;

	Scene* newScene = nullptr;

	switch (sceneType)
	{
		case SceneType::DevScene:
			_scene = make_unique<DevScene>();
			//newScene = new DevScene();
			break;
		case SceneType::EditScene:
			_scene = make_unique<EditScene>();
			//newScene = new EditScene();
			break;
	}

	//SAFE_DELETE(_scene);

	//_scene = newScene;
	_sceneType = sceneType;
	_scene->Init();

	
}
