#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Snail enemy 
struct Spider
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSpider(vec2 position, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createExplodingSpider(Motion givenMotion, ECS::Entity entity);
};
