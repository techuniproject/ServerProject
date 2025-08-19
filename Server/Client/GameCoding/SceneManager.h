#pragma once
//#include "Scene.h" //pch�� ���� ��� , �� ��� �߰��� ���߻��ϹǷ�
class Scene;

class SceneManager
{
	//2025-08-17 ������
	//DECLARE_SINGLE(SceneManager)
	DECLARE_DEFAULT_CONSTRUCTOR(SceneManager)
	DECLARE_DEFAULT_DESTRUCTOR(SceneManager)
	// �����Ϸ��� ������ִ� �⺻ �Ͻ��� �Ҹ��ڴ� ������� �����ϹǷ� 
	// ���漱������ Scene�� �˷��ְ�, unique_ptr<Scene>���� delete�ؾ��ϴ� �κп��� ������
	// �Ͻ��� �Ҹ��ڴ� �ζ������� ������� TU�� Scene.h�� �߰����� TU�� ���� ������ ���� Ȯ�� �ִٴ� ��
public:
	void Init();
	void Update();
	void Render(HDC hdc);

	void Clear();//�̰� �ʿ����� �ָ�

public:
	void ChangeScene(SceneType sceneType);
	//Scene* GetCurrentScene() { return _scene;} //�������� ��� ��
	//Scene* GetCurrentScene() { return _scene.get(); } �ܺο��� ������ ����
	//~SceneManager() = default; -> ���漱�� �� �ش� Ŭ������ ���� �Ҹ��� unique_ptr<Scene>��������Ϸ��ؼ� ���� ���⼭ x

	/*template<typename T>
	T* GetCurrentScene() noexcept
		requires is_base_of_v<Scene,T>
	{
		return dynamic_cast<T*>(_scene.get());
	}
	
	template<typename T>
	const T* GetCurrentScene() const noexcept
		requires is_base_of_v<Scene, T>
	{
		return dynamic_cast<const T*>(_scene.get());
	}*/

	SceneType& GetCurrentSceneType() { return _sceneType; }
	Scene& GetCurrentScene() { assert(_scene);  return *_scene; }
	const Scene& GetCurrentScene() const { assert(_scene);  return *_scene; }
	//Scene* GetCurrentScene1() { assert(_scene);  return _scene.get(); }
	
private:
	//Scene* _scene;
	unique_ptr<Scene> _scene;
	SceneType _sceneType = SceneType::None;

public:
	Vec2 GetCameraPos() { return _cameraPos; }
	void SetCameraPos(Vec2 pos) { _cameraPos = pos; }

private:
	Vec2 _cameraPos = {400, 300};
};

