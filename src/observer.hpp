#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "event.hpp"

class Observer
{
public:
	 ~Observer() {}
	virtual void onNotify(const ECS::Entity& entity, Event event) = 0;
};