#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Event {
public:
	enum EventType { COLLISION_EVENT };
	EventType type;
	ECS::Entity& other_entity;

	Event(EventType t, ECS::Entity& o) : type(t), other_entity(o) {}
};