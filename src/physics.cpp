// internal
#include "physics.hpp"
#include "projectile.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"

#include <memory>

// Returns the local bounding coordinates scaled by the current size of the entity 
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool collides(const Motion& motion1, const Motion& motion2)
{
	auto dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x/2.0f, 2.f) + std::pow(get_bounding_box(motion1).y/2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x/2.0f, 2.f) + std::pow(get_bounding_box(motion2).y/2.0f, 2.f));
	float r = max(other_r, my_r);
	if (dist_squared < r * r)
		return true;
	return false;
}

void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	
	//for (auto& motion : ECS::registry<Motion>.components)

    float step_seconds = 1.0f * (elapsed_ms / 1000.f);

    // move the projectile!
	for (auto entity: ECS::registry<Projectile>.entities)
	{
		auto& motion = ECS::registry<Motion>.get(entity);
		vec2 velocity = motion.velocity;
		motion.position += velocity * step_seconds;
	}

	// Move the entities who are not at their destinations
	auto& destReg = ECS::registry<Destination>;
	std::vector<std::shared_ptr<ECS::Entity>> toRemove;
	for (size_t i = 0; i < destReg.components.size(); i++)
    {
	    auto& entity = destReg.entities[i];
	    auto& dest = destReg.components[i];
	    auto& motion = ECS::registry<Motion>.get(entity);
        vec2 newPos = motion.position + (motion.velocity * step_seconds);
        if ((dot(motion.position - newPos, dest.position - newPos) > 0) || (dest.position == newPos))
        {
            // overshot or perfectly hit destination
            // set velocity back to 0 stop moving
            motion.position = dest.position;
            motion.velocity = {0.f, 0.f};
            toRemove.push_back(std::make_shared<ECS::Entity>(entity));
        }
        else
        {
            motion.position = newPos;
        }
    }
	// remove all that reached destination
	// someone tell me (emily) if this is dumb and if there's a better way to do this
	for (auto entityPtr : toRemove)
    {
	    destReg.remove(*entityPtr);
    }

	(void)window_size_in_game_units;

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
		for (auto& motion : ECS::registry<Motion>.components)
		{
			// draw a cross at the position of all objects
			auto scale_horizontal_line = motion.scale;
			scale_horizontal_line.y *= 0.1f;
			auto scale_vertical_line = motion.scale;
			scale_vertical_line.x *= 0.1f;
			DebugSystem::createLine(motion.position, scale_horizontal_line);
			DebugSystem::createLine(motion.position, scale_vertical_line);
		}
	}

	// Check for collisions between all moving entities
	auto& motion_container = ECS::registry<Motion>;
	// for (auto [i, motion_i] : enumerate(motion_container.components)) // in c++ 17 we will be able to do this instead of the next three lines
	for (unsigned int i=0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		ECS::Entity entity_i = motion_container.entities[i];
		for (unsigned int j=i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			ECS::Entity entity_j = motion_container.entities[j];

			if (collides(motion_i, motion_j))
			{
				//notify both entities of collision
				notify(entity_i, Event(Event::COLLISION_EVENT, entity_j));
				notify(entity_j, Event(Event::COLLISION_EVENT, entity_i));
			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}
