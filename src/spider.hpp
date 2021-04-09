#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "ai.hpp"

// Snail enemy 
struct Spider
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSpider(vec2 position, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createSpider(Motion motion, ECS::Entity entity = ECS::Entity(), std::shared_ptr<BTNode> tree = nullptr);
    static ECS::Entity createExplodingSpider(Motion givenMotion, ECS::Entity entity);
};

struct SuperSpider {
	static ECS::Entity createSuperSpider(vec2 position, ECS::Entity entity = ECS::Entity());
};
