#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon enemy 
struct Turtle
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createTurtle(vec2 position);
};
