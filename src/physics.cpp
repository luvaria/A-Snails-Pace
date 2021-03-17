// Header
#include "physics.hpp"
#include "geometry.hpp"
#include "projectile.hpp"
#include "tiles/wall.hpp"
#include "tiles/water.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"
#include "render.hpp"
#include "world.hpp"
#include "collectible.hpp"

// stlib
#include <memory>
#include <snail.hpp>
#include <spider.hpp>
#include <bird.hpp>
#include <iostream>
#include <cstdio>

// Returns the local bounding coordinates scaled by the current size of the entity 
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

void bounceProjectileOffWall(Motion& projectileMotion, std::vector<ColoredVertex> projectileVertices, std::vector<ColoredVertex> wallVertices)
{
	//first we get the bounding box cooordinates for the wall
	float minx = -1;
	float miny = -1;
	float maxx = -1;
	float maxy = -1;
	for (const auto& point : wallVertices)
	{
		auto pos = point.position;
		minx = (minx == -1) ? pos.x : min(minx, pos.x);
		miny = (miny == -1) ? pos.y : min(miny, pos.y);
		maxx = (maxx == -1) ? pos.x : max(maxx, pos.x);
		maxy = (maxy == -1) ? pos.y : max(maxy, pos.y);
	}

	auto bottomLeft = vec2(minx, maxy);
	auto bottomRight = vec2(maxx, maxy);
	auto upperRight = vec2(maxx, miny);
	auto upperLeft = vec2(minx, miny);

	std::vector<Geometry::Line> wallLines = std::vector<Geometry::Line>();
	Geometry::Line bottomLine = { bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y };
	Geometry::Line rightLine = { bottomRight.x, bottomRight.y, upperRight.x, upperRight.y };
	Geometry::Line upperLine = { upperRight.x, upperRight.y, upperLeft.x, upperLeft.y };
	Geometry::Line leftLine = { upperLeft.x, upperLeft.y, bottomLeft.x, bottomLeft.y };
	wallLines.push_back(bottomLine);
	wallLines.push_back(rightLine);
	wallLines.push_back(upperLine);
	wallLines.push_back(leftLine);

	//for each point which is inside the wall, determine the distance it takes along
	// the velocity to get it out of the bounding box.
	float maxDistance = 0;
	Geometry::Line reflectingLine{};
	for (const auto& point : projectileVertices)
	{
		if (Geometry::pointInsideConvexHull(vec2(point.position.x, point.position.y), wallLines))
		{
			auto backwardVelocity = vec2(projectileMotion.velocity.x * (-200), projectileMotion.velocity.y * (-200));
			auto point2 = vec2(point.position.x, point.position.y) + backwardVelocity;
			Geometry::Line projectileLine = { point.position.x, point.position.y, point2.x, point2.y };
			//go through every wall line and find the one which intersects, then calculate the distance to that point.
			bool foundWall = false;
			for (auto wallLine : wallLines)
			{
				if (Geometry::linesIntersect(projectileLine, wallLine))
				{
					assert(foundWall == false); //you should only intersect with one wallLine
					foundWall = true;
					//if they intersect, then this is the line that determines the distance my point needs to
					// go back. So, determine what that distance is.
					vec2 intersectionPoint = Geometry::intersectionOfLines(projectileLine, wallLine);
					assert(Geometry::pointIsOnLine(intersectionPoint, wallLine));

					float distance = glm::length(vec2(point.position.x, point.position.y) - intersectionPoint);
					if (distance > maxDistance)
					{
						maxDistance = distance;
						reflectingLine = wallLine;
					}
				}
			}
			assert(foundWall);
		}
	}
	// take the velocity as a unit vector, multiply it by the negative of the maxDistance, and add that to the position.
	float velocityLength = glm::length(projectileMotion.velocity);
	if (velocityLength != 0)
	{
		projectileMotion.position.x -= ((projectileMotion.velocity.x / velocityLength) * (maxDistance + 0.01)); //avoid collision
		projectileMotion.position.y -= ((projectileMotion.velocity.y / velocityLength) * (maxDistance + 0.01));
	}

	//reflect off the wall you colided with, which is either a vertical or a horizontal line.
	if (reflectingLine == bottomLine || reflectingLine == upperLine)
	{
		projectileMotion.velocity.y *= -1;
	}
	else
	{
		projectileMotion.velocity.x *= -1;
	}
}

bool collides(ECS::Entity& entity1, ECS::Entity& entity2, Motion& motion1, Motion& motion2, bool doPenetrationFree)
{
	//first we get bounding boxes, and see if the meshes have a chance to collide
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x / 2.0f, 2.f) + std::pow(get_bounding_box(motion1).y / 2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x / 2.0f, 2.f) + std::pow(get_bounding_box(motion2).y / 2.0f, 2.f));
	auto dp = motion1.position - motion2.position;
	auto lengthdp = glm::length(dp);
	auto radiusSum = other_r + my_r;
	if (radiusSum < lengthdp)
	{
		//bounding boxes don't collide, so no chance for meshes to.
		return false;
	}

	//now we get the vertices and indices for the two meshes, using the reduced meshes for faster detection.
	auto mesh1_ptr = ECS::registry<MinShadedMeshRef>.get(entity1).reference_to_cache;
	auto mesh2_ptr = ECS::registry<MinShadedMeshRef>.get(entity2).reference_to_cache;
	auto vertices1 = std::vector<ColoredVertex>(mesh1_ptr->mesh.vertices); //copy
	auto indices1 = mesh1_ptr->mesh.vertex_indices;
	auto vertices2 = std::vector<ColoredVertex>(mesh2_ptr->mesh.vertices); //copy
	auto indices2 = mesh2_ptr->mesh.vertex_indices;

	//we need to adjust the vertices for the position and scale and angle.
	Transform transform1;
	transform1.translate(motion1.position);
	transform1.scale(motion1.scale);
	transform1.rotate(motion1.angle);
	for (int i = 0; i < vertices1.size(); i++)
	{
		vec3 position = vec3(vertices1[i].position.x, vertices1[i].position.y, 1.0);
		vertices1[i].position = transform1.mat * position;
	}
	Transform transform2;
	transform2.translate(motion2.position);
	transform2.scale(motion2.scale);
	transform2.rotate(motion2.angle);
	for (int i = 0; i < vertices2.size(); i++)
	{
		vec3 position = vec3(vertices2[i].position.x, vertices2[i].position.y, 1.0);
		vertices2[i].position = transform2.mat * position;
	}

	//go over all the indices for the first mesh and make the lines between the vertices.
	std::vector<Geometry::Line> lines1 = std::vector<Geometry::Line>();
	for (int i = 0; i < indices1.size() - 1; i++)
	{
		auto vertex1 = vertices1[indices1[i]].position;
		auto vertex2 = vertices1[indices1[i + 1]].position;
		Geometry::Line line1 = { vertex1.x, vertex1.y, vertex2.x, vertex2.y }; //x0,y0,x1,y1
		lines1.push_back(line1);
	}
	auto vertex1 = vertices1[indices1.back()].position;
	auto vertex2 = vertices1[indices1[0]].position;
	Geometry::Line line1 = { vertex1.x, vertex1.y, vertex2.x, vertex2.y }; //x0,y0,x1,y1
	lines1.push_back(line1);

	//go over all the indices for the second mesh and make the lines between the vertices.
	std::vector<Geometry::Line> lines2 = std::vector<Geometry::Line>();
	for (int i = 0; i < indices2.size() - 1; i++)
	{
		auto vertex3 = vertices2[indices2[i]].position;
		auto vertex4 = vertices2[indices2[i + 1]].position;
		Geometry::Line line2 = { vertex3.x, vertex3.y, vertex4.x, vertex4.y }; //x0,y0,x1,y1
		lines2.push_back(line2);
	}
	auto vertex3 = vertices2[indices2.back()].position;
	auto vertex4 = vertices2[indices2[0]].position;
	Geometry::Line line2 = { vertex3.x, vertex3.y, vertex4.x, vertex4.y }; //x0,y0,x1,y1
	lines2.push_back(line2);

	//now we cover the case where one mesh is completely inside the other
	vec2 vertexa = vec2(vertices1[0].position.x, vertices1[0].position.y);
	vec2 vertexb = vec2(vertices2[0].position.x, vertices2[0].position.y);
	if (Geometry::pointInsideConvexHull(vertexa, lines2) || Geometry::pointInsideConvexHull(vertexb, lines1))
	{
		if (doPenetrationFree)
		{
			if (ECS::registry<Projectile>.has(entity1))
			{
				//projectile is entity1, wall is entity2
				bounceProjectileOffWall(motion1, vertices1, vertices2);
			}
			else
			{
				//projectile is entity2, wall is entity1
				bounceProjectileOffWall(motion2, vertices2, vertices1);
			}
		}
		return true;
	}

	// go over every pair of lines and see if they intersect.
	for (auto line3 : lines1)
	{
		for (auto line4 : lines2)
		{
			if (Geometry::linesIntersect(line3, line4)) 
			{
				if (doPenetrationFree)
				{
					if (ECS::registry<Projectile>.has(entity1))
					{
						//projectile is entity1, wall is entity2
						bounceProjectileOffWall(motion1, vertices1, vertices2);
					}
					else
					{
						//projectile is entity2, wall is entity1
						bounceProjectileOffWall(motion2, vertices2, vertices1);
					}
				}
				return true;
			}
		}
	}

	return false;
}

//returns true if entity_i and entity_j is a (Projectile,WallTile) pair in either order
bool isProjectileAndWall(ECS::Entity entity_i, ECS::Entity entity_j)
{
	return (ECS::registry<Projectile>.has(entity_i) && ECS::registry<WallTile>.has(entity_j)) ||
		(ECS::registry<WallTile>.has(entity_i) && ECS::registry<Projectile>.has(entity_j));
}

//returns true if we have a possibility of caring if entity_i and entity_j collide
//was edited to check collision with bird
bool ShouldCheckCollision(ECS::Entity entity_i, ECS::Entity entity_j)
{
	bool isValidSnailCollision_i = ECS::registry<Snail>.has(entity_i) &&
		(ECS::registry<Spider>.has(entity_j) || ECS::registry<WaterTile>.has(entity_j) || ECS::registry<Bird>.has(entity_j) || ECS::registry<Collectible>.has(entity_j));

	bool isValidSnailCollision_j = ECS::registry<Snail>.has(entity_j) &&
		(ECS::registry<Spider>.has(entity_i) || ECS::registry<WaterTile>.has(entity_i) || ECS::registry<Bird>.has(entity_i) || ECS::registry<Collectible>.has(entity_i));

	bool isValidProjectileCollision_i = ECS::registry<Projectile>.has(entity_i) &&
		(ECS::registry<Spider>.has(entity_j) || ECS::registry<WallTile>.has(entity_j) || ECS::registry<Bird>.has(entity_j));

	bool isValidProjectileCollision_j = ECS::registry<Projectile>.has(entity_j) &&
		(ECS::registry<Spider>.has(entity_i) || ECS::registry<WallTile>.has(entity_i) || ECS::registry<Bird>.has(entity_i));
	

	return isValidSnailCollision_i || isValidSnailCollision_j || isValidProjectileCollision_i || isValidProjectileCollision_j;
}

void stepToDestination(ECS::Entity entity, float step_seconds)
{
    auto& destReg = ECS::registry<Destination>;
    if (destReg.has(entity))
    {
        auto& dest = destReg.get(entity);
        auto& motion = ECS::registry<Motion>.get(entity);
        vec2 newPos = motion.position + (motion.velocity * step_seconds);
        if ((dot(motion.position - newPos, dest.position - newPos) > 0) || (dest.position == newPos))
        {
            // overshot or perfectly hit destination
            // set velocity back to 0 stop moving
            motion.position = dest.position;
            //motion.velocity = { 0.f, 0.f };
            destReg.remove(entity);
        }
        else
        {
            motion.position = newPos;
        }
    }
}

void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    (void)window_size_in_game_units;

    // Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.

	float step_seconds = 1.0f * (elapsed_ms / 1000.f);

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
    if (turnType == PLAYER_WAITING)
    {
        // Projectile previews
        for (auto entity : ECS::registry<Projectile::Preview>.entities)
        {
            auto& motion = ECS::registry<Motion>.get(entity);
            vec2 velocity = motion.velocity;
            motion.position += velocity * step_seconds;
        }
    }
    // if snail is moving to its destination then nothing else should be! for now...
	else if (turnType == PLAYER_UPDATE)
    {
        auto& snailEntity = ECS::registry<Snail>.entities[0];
	    stepToDestination(snailEntity, step_seconds);
    }
	else if (turnType == ENEMY)
    {
        // move the projectile!
	    // There shouldn't be any previews at this point, but to be safe
	    Projectile::Preview::removeCurrent();
        for (auto entity : ECS::registry<Projectile>.entities)
        {
            auto& motion = ECS::registry<Motion>.get(entity);
            vec2 velocity = motion.velocity;
            motion.position += velocity * step_seconds;
        }

        // move enemies
        // eventually this could be an enemy registry, but we only have spiders for now
        for (auto entity : ECS::registry<Spider>.entities)
        {
            stepToDestination(entity, step_seconds);
        }

		for (auto entity : ECS::registry<Bird>.entities) 
		{
			stepToDestination(entity, step_seconds);
		}
    }
	else if (turnType == CAMERA)
    {
        // move camera
        auto &cameraEntity = ECS::registry<Camera>.entities[0];
        stepToDestination(cameraEntity, step_seconds);
    }

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
		auto& motion_container = ECS::registry<Motion>;
		for (int i = motion_container.components.size() - 1; i >= 0; i--)
		{
			Motion& motion = motion_container.components[i];
			ECS::Entity entity = motion_container.entities[i];

			// ignore overlays
			if (ECS::registry<Overlay>.has(entity))
				continue;

			// ignore backgrounds
			if (ECS::registry<Parallax>.has(entity))
				continue;

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
	for (unsigned int i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		ECS::Entity entity_i = motion_container.entities[i];
		for (unsigned int j = i + 1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			ECS::Entity entity_j = motion_container.entities[j];
			if (ShouldCheckCollision(entity_i, entity_j))
			{
				bool doPenetrationFree = isProjectileAndWall(entity_i, entity_j);
				if (collides(entity_i, entity_j, motion_i, motion_j, doPenetrationFree))
				{
					//notify both entities of collision
					notify(Event(Event::COLLISION, entity_i, entity_j));
					notify(Event(Event::COLLISION, entity_j, entity_i));
				}
			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}
