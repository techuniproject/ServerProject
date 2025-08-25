#pragma once
//#include "Scene.h" //pch로 뺄지 고민 , 이 헤더 추가시 비용발생하므로
class Scene;
class MyPlayer;

class SceneManager
{
	//2025-08-17 서정원
	//DECLARE_SINGLE(SceneManager)
	DECLARE_DEFAULT_CONSTRUCTOR(SceneManager)
	DECLARE_DEFAULT_DESTRUCTOR(SceneManager)
	// 컴파일러가 만들어주는 기본 암시적 소멸자는 헤더에서 정의하므로 
	// 전방선언으로 Scene을 알려주고, unique_ptr<Scene>같이 delete해야하는 부분에서 오류남
	// 암시적 소멸자는 인라인으로 만들어져 TU가 Scene.h를 추가안한 TU에 한해 컴파일 에러 확률 있다는 것
public:
	void Init();
	void Update();
	void Render(HDC hdc);

	void Clear();//이게 필요한지 애매

public:
	void ChangeScene(SceneType sceneType);
	//Scene* GetCurrentScene() { return _scene;} //생포인터 사용 시
	//Scene* GetCurrentScene() { return _scene.get(); } 외부에서 삭제될 위험
	//~SceneManager() = default; -> 전방선언 후 해당 클래스에 대한 소멸을 unique_ptr<Scene>에서사용하려해서 정의 여기서 x

	template<typename T>
	T* GetCurrentScene() noexcept;

	template<typename T>
	const T* GetCurrentScene() const noexcept;


	SceneType& GetCurrentSceneType() { return _sceneType; }
	Scene& GetCurrentScene() { assert(_scene);  return *_scene; }
	const Scene& GetCurrentScene() const { assert(_scene);  return *_scene; }
	//Scene* GetCurrentScene1() { assert(_scene);  return _scene.get(); }
	
	shared_ptr<MyPlayer> GetMyPlayer() { return _myPlayer.lock(); }
	uint64 GetMyPlayerId();
	void SetMyPlayer(shared_ptr<MyPlayer> myPlayer) { _myPlayer = myPlayer; }

private:
	//Scene* _scene;
	unique_ptr<Scene> _scene;
	SceneType _sceneType = SceneType::None;
	weak_ptr<MyPlayer> _myPlayer;

public:
	Vec2 GetCameraPos() { return _cameraPos; }
	void SetCameraPos(Vec2 pos) { _cameraPos = pos; }

private:
	Vec2 _cameraPos = {400, 300};
};

#include "SceneManager.inl" // ⬅️ 템플릿 '정의'를 헤더 끝에 포함