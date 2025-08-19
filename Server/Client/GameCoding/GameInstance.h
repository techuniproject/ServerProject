#pragma once

class ObjectManager;
class InputManager;
class ResourceManager;
class SceneManager;
class SoundManager;
class TimeManager;
class Scene;
class ResourceBase;
class Texture;
class Sprite;
class Flipbook;
class Tilemap;
class Sound;
class GameObject;



// 2025-08-19 GameInstance 설계
class GameInstance
{
public:
	DECLARE_SINGLE(GameInstance)
	DECLARE_DEFAULT_CONSTRUCTOR(GameInstance);
	DECLARE_DEFAULT_DESTRUCTOR(GameInstance)

public:
	void Init(HWND hwnd);
	void Update();
	void Clear();

public: //SceneManager
	template<typename T>
	T* GetCurrentScene() noexcept;

	template<typename T>
	const T* GetCurrentScene() const noexcept;

	void RenderScene(HDC hdc);
	SceneType& GetCurrentSceneType();
	void ChangeScene(SceneType sceneType);
	Scene& GetCurrentScene();
	const Scene& GetCurrentScene()const;
	Vec2 GetCameraPos();
	void SetCameraPos(Vec2 pos);
	void ClearScene();

public:
	//TimeManager
	uint32 GetFps();
	float GetDeltaTime();

public:
	//InputManager
	bool GetButton(KeyType key);
	bool GetButtonDown(KeyType key);
	bool GetButtonUp(KeyType key);
	POINT GetMousePos();

public:
	//ResourceManager
	const fs::path& GetResourcePath();
	Texture* GetTexture(const wstring& key);
	Texture* LoadTexture(const wstring& key, const wstring& path, uint32 transparent = RGB(255, 0, 255));
	Sprite* GetSprite(const wstring& key);
	Sprite* CreateSprite(const wstring& key, Texture* texture, int32 x = 0, int32 y = 0, int32 cx = 0, int32 cy = 0);

	Flipbook* GetFlipbook(const wstring& key);
	Flipbook* CreateFlipbook(const wstring& key);

	Tilemap* GetTilemap(const wstring& key);
	Tilemap* CreateTilemap(const wstring& key);
	void SaveTilemap(const wstring& key, const wstring& path);
	Tilemap* LoadTilemap(const wstring& key, const wstring& path);

	Sound* GetSound(const wstring& key);
	Sound* LoadSound(const wstring& key, const wstring& path);
public:
		//SoundManager
	void Play(const wstring& key, bool loop = false);
	LPDIRECTSOUND GetSoundDevice();
private:
	unique_ptr<ObjectManager> _ObjectManager;
	unique_ptr<InputManager> _InputManager;
	unique_ptr<ResourceManager> _ResourceManager;
	unique_ptr<SceneManager> _SceneManager;
	unique_ptr<SoundManager> _SoundManager;
	unique_ptr<TimeManager> _TimeManager;
};

#include "GameInstance.inl" // ⬅️ 포워딩 템플릿 '정의' 포함