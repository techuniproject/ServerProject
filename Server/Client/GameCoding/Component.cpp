#include "pch.h"
#include "Component.h"

Component::Component()
{

}

Component::~Component()
{
}

shared_ptr<Actor> Component::GetOwner()const
{
	
	//if (shared_ptr<Actor>actor =_owner.lock()) {
	//	return actor;
	//}
	//return nullptr;
	return _owner.lock();
}
