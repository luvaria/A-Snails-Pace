#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Projectile
{
	// tags all projectile types
};

struct SnailProjectile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createProjectile(vec2 position, vec2 velocity, bool preview = false);
	static ECS::Entity createProjectile(Motion motion, bool preview = false);

	struct Preview
	{
	    static void removeCurrent();
	};
};

struct SlugProjectile {
	static ECS::Entity createProjectile(vec2 position, vec2 velocity);
    static ECS::Entity createProjectile(Motion motion);
};