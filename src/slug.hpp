#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "ai.hpp"

// Snail enemy 
struct Slug
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSlug(vec2 position, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createSlug(Motion motion, ECS::Entity entity = ECS::Entity(), std::shared_ptr<BTNode> tree = nullptr);
};