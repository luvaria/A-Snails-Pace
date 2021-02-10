#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct WaterTile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createWaterTile(vec2 pos);
};
