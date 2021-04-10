#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Snail
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSnail(vec2 pos, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createSnail(Motion motion, ECS::Entity entity = ECS::Entity());
};
