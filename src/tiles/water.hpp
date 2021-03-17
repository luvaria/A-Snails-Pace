#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"

struct WaterTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createWaterTile(Tile& tile, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createWaterTile(vec2 pos, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createWaterSplashTile(Tile& tile, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createWaterSplashTile(vec2 pos, ECS::Entity entity = ECS::Entity());
    static int splashEntityID;
    ECS::Entity entity;
    static void onNotify(Event env, ECS::Entity& e);
};
