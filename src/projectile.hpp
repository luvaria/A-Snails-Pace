#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// snail projectile
struct Projectile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createProjectile(vec2 position, vec2 velocity, bool preview = false);

	struct Preview
	{
	    static void removeCurrent();
	};
};