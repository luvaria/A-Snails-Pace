#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Projectile
{
	// tags all projectile types
    int moved = 0;
    vec2 origScale = vec2(20,-20);
    static int snailProjectileMaxMoves;
    static int aiProjectileMaxMoves;
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
