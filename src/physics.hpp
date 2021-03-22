#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "subject.hpp"

#include <random>
#include <functional>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem : public Subject
{
public:
    
    std::default_random_engine rng;
    std::uniform_real_distribution<float> uniform_dist;
    
    bool randomBool() {
        static auto gen = std::bind(std::uniform_int_distribution<>(0,1),std::default_random_engine());
        bool b = gen();
        return b;
    }
    
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Stucture to store collision information
	struct Collision
	{
		// Note, the first object is stored in the ECS container.entities
		ECS::Entity other; // the second object involved in the collision
		Collision(ECS::Entity& other);
	};

private:
	void SetupCornerMovement(ECS::Entity entity, Destination& dest);
	void SetupNextCornerSegment(Motion& motion);
	vec2 StepSnailAroundCorner(float step_seconds, Motion& motion);
	void stepToDestinationAroundCorner(ECS::Entity entity, float step_seconds);
	void stepToDestination(ECS::Entity entity, float step_seconds);

	//fields for rounding the corner

	float t = 0; // t between 0 and 1 for rounding the corner.
	bool AreRoundingCorner = false;
	float total_time; //total elapsed_time that it should take to round the corner
	float theta_old = 0; //represents the angle that the previous time step had
	float segment_angle = 0; //angle for the previous segment
	float dest_angle;
	int numSegmentsCompleted = 0;
	int numSegments = 0;
	bool isSegmentAngleIncreasing = true;
	//coefficients of the basis functions for stepping around the corner
	vec2 P0;
	vec2 P1;
	vec2 T0;
	vec2 T1;
};
