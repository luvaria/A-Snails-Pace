#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct GroundTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createGroundTile(vec2 pos);
};
