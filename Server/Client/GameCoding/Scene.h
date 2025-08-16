#pragma once

class Actor;
class Creature;
class UI;

/*
추상 클래스는 인스턴스를 만들 수 없다.
그렇기 때문에 Scene같은 추상 클래스는 값으로 반환을 할 수 없다.
값으로 반환하는건 
*/

class Scene
{
public:
	Scene();
	virtual ~Scene();

	virtual void Init() abstract;
	virtual void Update() abstract;
	virtual void Render(HDC hdc) abstract;

	virtual void AddActor(Actor* actor);
	virtual void RemoveActor(Actor* actor);

	Creature* GetCreatureAt(Vec2Int cellPos);

public:
	vector<Actor*> _actors[LAYER_MAXCOUNT];
	vector<UI*> _uis;
};

