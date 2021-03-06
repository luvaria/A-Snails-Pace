#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"

class VineTile: Observer
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createVineTile(Tile& tile, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createVineTile(vec2 pos, ECS::Entity entity = ECS::Entity());
	void onNotify(Event env);
	ECS::Entity entity;
};
