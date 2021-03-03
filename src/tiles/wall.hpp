#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"

struct WallTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createWallTile(Tile& tile, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createWallTile(vec2 pos, ECS::Entity entity = ECS::Entity());
};
