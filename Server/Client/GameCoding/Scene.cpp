#include "pch.h"
#include "GameInstance.h"
#include "Scene.h"
#include "Actor.h"
#include "Creature.h"
#include "UI.h"
#include "TimeManager.h"
#include "SceneManager.h"

Scene::Scene()
{

}

Scene::~Scene()
{
	/*for (const vector<shared_ptr<Actor>>& actors : _actors)
		for (shared_ptr<Actor> actor : actors)
			SAFE_DELETE(actor);*/

	_actors->clear();

	//for (shared_ptr<UI> ui : _uis)
	//	SAFE_DELETE(ui);

	_uis.clear();
}

void Scene::Init()
{
	for (const vector<shared_ptr<Actor>>& actors : _actors)
		for (const shared_ptr<Actor>& actor : actors)
			actor->BeginPlay();

	for (shared_ptr<UI> ui : _uis)
		ui->BeginPlay();
}

void Scene::Update()
{
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	// บนป็
	for (const vector<shared_ptr<Actor>> actors : _actors)
		for (shared_ptr<Actor> actor : actors)
			actor->Tick();

	for (shared_ptr<UI> ui : _uis)
		ui->Tick();
}

void Scene::Render(HDC hdc)
{
	vector<shared_ptr<Actor>>& actors = _actors[LAYER_OBJECT];
	sort(actors.begin(), actors.end(), [=](shared_ptr<Actor>& a, shared_ptr<Actor>& b)
	{
		return a->GetPos().y < b->GetPos().y;
	});

	for (const vector<shared_ptr<Actor>>& actors : _actors)
		for (shared_ptr<Actor> actor : actors)
			actor->Render(hdc);

	for (shared_ptr<UI>& ui : _uis)
		ui->Render(hdc);
}

void Scene::AddActor(shared_ptr<Actor> actor)
{
	if (actor == nullptr)
		return;

	_actors[actor->GetLayer()].push_back(actor);
}

void Scene::RemoveActor(shared_ptr<Actor> actor)
{
	if (actor == nullptr)
		return;

	vector<shared_ptr<Actor>>& v = _actors[actor->GetLayer()];
	v.erase(std::remove(v.begin(), v.end(), actor), v.end());
}

shared_ptr<Creature> Scene::GetCreatureAt(Vec2Int cellPos)
{
	for (shared_ptr<Actor>& actor : _actors[LAYER_OBJECT])
	{
		// GameObjectType
		shared_ptr<Creature> creature = dynamic_pointer_cast<Creature>(actor);
		if (creature && creature->GetCellPos() == cellPos)
			return creature;
	}

	return nullptr;
}
