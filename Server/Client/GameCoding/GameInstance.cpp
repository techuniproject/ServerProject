#include "pch.h"
#include "GameInstance.h"
#include "ObjectManager.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "Texture.h"
#include "Sprite.h"
#include "Flipbook.h"
#include "Tilemap.h"
#include "Sound.h"
#include "Scene.h"

DEFINE_DEFAULT_CONSTRUCTOR(GameInstance); //unique_ptr때문에 헤더를 알아야 메모리 관리 코드 만들 수 있음
DEFINE_DEFAULT_DESTRUCTOR(GameInstance);




void GameInstance::Init(HWND hwnd) 
{

	_ObjectManager = make_unique<ObjectManager>();
	_InputManager = make_unique<InputManager>();
	_ResourceManager = make_unique<ResourceManager>();
	_SceneManager = make_unique<SceneManager>();
	_SoundManager = make_unique<SoundManager>();
	_TimeManager = make_unique<TimeManager>();
	_NetworkManager = make_unique<NetworkManager>();

	/*_ObjectManager = make_shared<ObjectManager>();
	_InputManager = make_shared<InputManager>();
	_ResourceManager = make_shared<ResourceManager>();
	_SceneManager = make_shared<SceneManager>();
	_SoundManager = make_shared<SoundManager>();
	_TimeManager = make_shared<TimeManager>();*/

	_TimeManager->Init();
	_InputManager->Init(hwnd);
	_SceneManager->Init();
	_ResourceManager->Init(hwnd, fs::path(L"C:\\Users\\서정원\\Desktop\\ServerClient\\ServerProject\\Server\\Client\\Resources"));
	_SoundManager->Init(hwnd);
	_NetworkManager->Init();
}

void GameInstance::Update() 
{
	_TimeManager->Update();
	_InputManager->Update();
	_SceneManager->Update();
	_NetworkManager->Update();
}

void GameInstance::Clear()
{
	_SceneManager->Clear();
	_ResourceManager->Clear();

	_SceneManager.reset();
	_ResourceManager.reset();
	_SoundManager.reset();
	_TimeManager.reset();
	_InputManager.reset();
	_ObjectManager.reset();
}



//SceneManager
void GameInstance::RenderScene(HDC hdc)
{
	_SceneManager->Render(hdc);
}

void GameInstance::ClearScene()
{
	_SceneManager->Clear();
}

SceneType& GameInstance::GetCurrentSceneType() 
{
	return _SceneManager->GetCurrentSceneType();
}

void GameInstance::ChangeScene(SceneType sceneType)
{
	_SceneManager->ChangeScene(sceneType);
}

Scene& GameInstance::GetCurrentScene()
{
	return _SceneManager->GetCurrentScene();
}

const Scene& GameInstance::GetCurrentScene()const 
{
	return _SceneManager->GetCurrentScene();
}

Vec2 GameInstance::GetCameraPos() 
{
	return _SceneManager->GetCameraPos();
}

void GameInstance::SetCameraPos(Vec2 pos) 
{
	return _SceneManager->SetCameraPos(pos);
}

//TimeManager

uint32 GameInstance::GetFps()
{
	return _TimeManager->GetFps();
}

float GameInstance::GetDeltaTime()
{
	return _TimeManager->GetDeltaTime();
}

//InputManager

bool GameInstance::GetButton(KeyType key) 
{
	return _InputManager->GetButton(key);
}

bool GameInstance::GetButtonDown(KeyType key)
{
	return _InputManager->GetButtonDown(key);
}

bool GameInstance::GetButtonUp(KeyType key)
{
	return _InputManager->GetButtonUp(key);
}

POINT GameInstance::GetMousePos()
{
	return _InputManager->GetMousePos();
}

//ResourceManager

const fs::path& GameInstance::GetResourcePath()
{
	return  _ResourceManager->GetResourcePath();
}

shared_ptr<Texture> GameInstance::GetTexture(const wstring& key)
{
	return  _ResourceManager->GetTexture(key);

}

shared_ptr<Texture> GameInstance::LoadTexture(const wstring& key, const wstring& path, uint32 transparent /*= RGB(255, 0, 255)*/)
{
	return  _ResourceManager->LoadTexture(key,path,transparent);
}

shared_ptr<Sprite> GameInstance::GetSprite(const wstring& key)
{
	return  _ResourceManager->GetSprite(key);
}

shared_ptr<Sprite> GameInstance::CreateSprite(const wstring& key, shared_ptr<Texture> texture, int32 x, int32 y, int32 cx, int32 cy)
{
	return  _ResourceManager->CreateSprite(key, texture, x, y, cx, cy);
}

shared_ptr<Flipbook> GameInstance::GetFlipbook(const wstring& key)
{
	return  _ResourceManager->GetFlipbook(key);
}

shared_ptr<Flipbook> GameInstance::CreateFlipbook(const wstring& key)
{
	return _ResourceManager->CreateFlipbook(key);
}

shared_ptr<Tilemap> GameInstance::GetTilemap(const wstring& key)
{
	return _ResourceManager->GetTilemap(key);
}

shared_ptr<Tilemap> GameInstance::CreateTilemap(const wstring& key)
{
	return _ResourceManager->CreateTilemap(key);
}

void GameInstance::SaveTilemap(const wstring& key, const wstring& path)
{
	 _ResourceManager->SaveTilemap(key, path);
}

shared_ptr<Tilemap> GameInstance::LoadTilemap(const wstring& key, const wstring& path)
{
	return _ResourceManager->LoadTilemap(key, path);
}

shared_ptr<Sound> GameInstance::GetSound(const wstring& key)
{
	return _ResourceManager->GetSound(key);
}

shared_ptr<Sound> GameInstance::LoadSound(const wstring& key, const wstring& path)
{
	return _ResourceManager->LoadSound(key, path);
}

// SoundManager

void GameInstance::Play(const wstring& key, bool loop)
{
	_SoundManager->Play(key, loop);
}

LPDIRECTSOUND GameInstance::GetSoundDevice()
{
	return _SoundManager->GetSoundDevice();
}