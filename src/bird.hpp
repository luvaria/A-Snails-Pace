#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Snail enemy 
struct Bird
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createBird(vec2 position, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createBird(Motion motion, ECS::Entity entity = ECS::Entity());
};