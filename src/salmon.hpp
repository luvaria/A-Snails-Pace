#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Salmon
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSalmon(vec2 pos);
};
