#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct WallTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createWallTile(vec2 pos);
};
