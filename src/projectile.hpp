#pragma once

#include <chrono>
#include "common.hpp"
#include "tiny_ecs.hpp"

// snail projectile
struct Projectile
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createProjectile(vec2 position, vec2 velocity, bool preview = false);

	struct Preview
	{
	    static std::chrono::time_point<std::chrono::high_resolution_clock> s_can_show_projectile_preview_time;
	    static vec2 s_projectile_preview_pos;
	};
};