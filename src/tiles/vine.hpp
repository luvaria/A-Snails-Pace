#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct VineTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createVineTile(vec2 pos);
};
