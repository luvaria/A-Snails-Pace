#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"

struct VineTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createVineTile(Tile& tile, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createVineTile(vec2 pos, ECS::Entity entity = ECS::Entity());
};
