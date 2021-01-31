#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct Fish
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createFish(vec2 position);
};
