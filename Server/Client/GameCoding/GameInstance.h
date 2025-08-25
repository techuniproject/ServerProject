#pragma once

class ObjectManager;
class InputManager;
class ResourceManager;
class SceneManager;
class SoundManager;
class TimeManager;
class NetworkManager;
class Scene;
class ResourceBase;
class Texture;
class Sprite;
class Flipbook;
class Tilemap;
class Sound;
class GameObject;
class DevScene;
class MyPlayer;



// 2025-08-19 GameInstance 설계
class GameInstance
{
public:
	DECLARE_SINGLE(GameInstance)
	DECLARE_DEFAULT_CONSTRUCTOR(GameInstance);
	DECLARE_DEFAULT_DESTRUCTOR(GameInstance)
	/*
	unique_ptr을 사용 시, 헤더에 생성자/소멸자 정의하면 delete와 같은 메모리 구현부에서
	직접 ptr타입을 알아야하므로 헤더가 추가되어야하지만, 정의를 cpp에서 하면 문제 방지.
	*/


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
	shared_ptr<MyPlayer> GetMyPlayer();
	uint64 GetMyPlayerId();
	void SetMyPlayer(shared_ptr<MyPlayer> myPlayer);

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
	shared_ptr<Texture> GetTexture(const wstring& key);
	shared_ptr<Texture> LoadTexture(const wstring& key, const wstring& path, uint32 transparent = RGB(255, 0, 255));
	shared_ptr<Sprite> GetSprite(const wstring& key);
	shared_ptr<Sprite> CreateSprite(const wstring& key, shared_ptr<Texture> texture, int32 x = 0, int32 y = 0, int32 cx = 0, int32 cy = 0);

	shared_ptr<Flipbook> GetFlipbook(const wstring& key);
	shared_ptr<Flipbook> CreateFlipbook(const wstring& key);

	shared_ptr<Tilemap> GetTilemap(const wstring& key);
	shared_ptr<Tilemap> CreateTilemap(const wstring& key);
	void SaveTilemap(const wstring& key, const wstring& path);
	shared_ptr<Tilemap> LoadTilemap(const wstring& key, const wstring& path);

	shared_ptr<Sound> GetSound(const wstring& key);
	shared_ptr<Sound> LoadSound(const wstring& key, const wstring& path);
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
	unique_ptr<NetworkManager> _NetworkManager;
};

#include "GameInstance.inl" // ⬅️ 포워딩 템플릿 '정의' 포함