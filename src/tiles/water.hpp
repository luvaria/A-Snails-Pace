#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"

struct WaterTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createWaterTile(Tile& tile, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createWaterTile(vec2 pos, ECS::Entity entity = ECS::Entity());
};
