// internal
#include "physics.hpp"
#include "geometry.hpp"
#include "render.hpp"
#include "world.hpp"
#include "debug.hpp"
#include "snail.hpp"
#include "slug.hpp"
#include "spider.hpp"
#include "projectile.hpp"
#include "tiles/wall.hpp"
#include "tiles/water.hpp"
#include "collectible.hpp"
#include "particle.hpp"

// stlib
#include <memory>
#include <iostream>
#include <cstdio>
#include <math.h>

// Returns the local bounding coordinates scaled by the current size of the entity 
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

//given that the entity is moving from oldPos to newPos, update any tiles occupancy status
void UpdateTileOccupancy(vec2 oldPos, vec2 newPos)
{
	auto scale = TileSystem::getScale();
	int oldPosxCoord = static_cast<int>(oldPos.x / scale); //index into the tiles array
	int oldPosyCoord = static_cast<int>(oldPos.y / scale);
	int newPosxCoord = static_cast<int>(newPos.x / scale);
	int newPosyCoord = static_cast<int>(newPos.y / scale);
	//if the tiles are the same, it didn't change tiles, so we don't do anything
	if (oldPosxCoord != newPosxCoord || oldPosyCoord != newPosyCoord)
	{
		auto& tiles = TileSystem::getTiles();
		auto& oldTile = tiles[oldPosyCoord][oldPosxCoord];
		auto& newTile = tiles[newPosyCoord][newPosxCoord];
		oldTile.removeOccupyingEntity();
		newTile.addOccupyingEntity();
	}
}

float ComputeFinalAngle(float initial_angle, bool isAngleIncreasing) 
{
	if (isAngleIncreasing) 
	{
		if (initial_angle == 0) { return PI / 2; }
		else if (initial_angle == PI / 2) { return PI; }
		else if (initial_angle == PI) { return - PI / 2; }
		else if (initial_angle == - PI/2) { return 0; }
	}
	else 
	{
		if (initial_angle == 0) { return -PI / 2; }
		else if (initial_angle == PI / 2) { return 0; }
		else if (initial_angle == PI) { return PI / 2; }
		else if (initial_angle == -PI / 2) { return PI; }
	}
	return 0;
}

//when this function is called, we will create the CornerMovement struct, and intialize it
void PhysicsSystem::SetupCornerMovement(ECS::Entity entity, Destination& dest)
{
	ECS::registry<CornerMotion>.emplace(entity);
	auto& cornerMotion = ECS::registry<CornerMotion>.get(entity);
	cornerMotion.numSegmentsCompleted = 0;
	auto& direction = ECS::registry<DirectionInput>.get(entity).direction;
	auto& motion = ECS::registry<Motion>.get(entity);
	switch (direction) 
	{
	case DIRECTION_NORTH: 
		{
			cornerMotion.segment_angle = PI / 2;
			cornerMotion.isSegmentAngleIncreasing = dest.position.x < motion.position.x;
			motion.lastDirection = cornerMotion.isSegmentAngleIncreasing ? DIRECTION_WEST : DIRECTION_EAST;
			break;
		}
	case DIRECTION_SOUTH:
		{
			cornerMotion.segment_angle = 3 * PI / 2;
			cornerMotion.isSegmentAngleIncreasing = dest.position.x > motion.position.x;
			motion.lastDirection = cornerMotion.isSegmentAngleIncreasing ? DIRECTION_EAST : DIRECTION_WEST;
			break;
		}
	case DIRECTION_EAST:
		{
			cornerMotion.segment_angle = 0;
			cornerMotion.isSegmentAngleIncreasing = dest.position.y < motion.position.y;
			motion.lastDirection = cornerMotion.isSegmentAngleIncreasing ? DIRECTION_NORTH : DIRECTION_SOUTH;
			break;
		}
	case DIRECTION_WEST:
		{
			cornerMotion.segment_angle = PI;
			cornerMotion.isSegmentAngleIncreasing = dest.position.y > motion.position.y;
			motion.lastDirection = cornerMotion.isSegmentAngleIncreasing ? DIRECTION_SOUTH : DIRECTION_NORTH;
			break;
		}
	}
	cornerMotion.dest_angle = ComputeFinalAngle(motion.angle, !cornerMotion.isSegmentAngleIncreasing); //we subtract angles from snail angle, so feed in the opposite here.
	cornerMotion.theta_old = cornerMotion.segment_angle;
}

//requires there to be a next segment
void PhysicsSystem::SetupNextCornerSegment(ECS::Entity entity, Motion& motion)
{
	auto& cornerMotion = ECS::registry<CornerMotion>.get(entity);
	float segmentAngleFactor = cornerMotion.isSegmentAngleIncreasing ? 1.0f : -1.0f;
	cornerMotion.numSegments = 9;
	//we change the angle if we have completed 3,4,5, or 6 segments.
	if (3 <= cornerMotion.numSegmentsCompleted && cornerMotion.numSegmentsCompleted <= 6)
	{
		cornerMotion.segment_angle += (PI / 8) * segmentAngleFactor; //if the angle stays the same, then you will have a straight segment!
	}

	//then we use this angle to determine P0,P1,T0,T1

	auto scale = TileSystem::getScale();

	//compute the next point that we want to go to
	auto magnitude = scale / 5;
	auto delta_position = vec2(magnitude * cos(cornerMotion.segment_angle), -magnitude * sin(cornerMotion.segment_angle));
	auto temp_dest = motion.position + delta_position;

	//now get the tangents
	if (cornerMotion.numSegmentsCompleted == 0)
	{
		//first segment, so T0 goes in direction of segment_angle
		cornerMotion.T0 = vec2(magnitude * cos(cornerMotion.segment_angle), -magnitude * sin(cornerMotion.segment_angle));
	}
	else 
	{
		//T0 is always equal to the previous T1, unless we are the first segmnet, which is already handled
		cornerMotion.T0 = cornerMotion.T1; //previous T1
	}

	if (cornerMotion.numSegmentsCompleted == 8)
	{
		//last segment, so T1 is the same as before.
	}
	else 
	{
		// get the next next point
		auto temp_angle = cornerMotion.segment_angle;
		if (2 <= cornerMotion.numSegmentsCompleted && cornerMotion.numSegmentsCompleted <= 5)
		{
			temp_angle += (PI / 8) * segmentAngleFactor;
		}
		auto delta_position_2 = vec2(magnitude * cos(temp_angle), -magnitude * sin(temp_angle));
		auto next_next_point = temp_dest + delta_position_2;
		//compute the line between position and next_next_point.
		cornerMotion.T1 = next_next_point - motion.position;
		//now need to make it's magnitude equal to 'magnitude'
		auto length = glm::length(cornerMotion.T1);
		cornerMotion.T1.x *= (magnitude / length); //gotta keep this small or it gets wavy
		cornerMotion.T1.y *= (magnitude / length);
	}

	cornerMotion.total_time = magnitude * 2.0f / glm::length(motion.velocity);
	cornerMotion.t = 0.0f;
	cornerMotion.P0 = motion.position;
	cornerMotion.P1 = temp_dest;
}

//returns the position of the snail going around the corner.
//and adjust the angle.
vec2 PhysicsSystem::StepAroundCorner(ECS::Entity entity, float step_seconds, Motion& motion)
{
	auto& cornerMotion = ECS::registry<CornerMotion>.get(entity);
	cornerMotion.t += step_seconds / cornerMotion.total_time;
	cornerMotion.t = min(cornerMotion.t, 1.0f); // don't let t > 1
	auto t = cornerMotion.t;
	//compute C(t) and set the position to it
	float h_00 = t * t * (2 * t - 3) + 1;
	float h_01 = -t * t * (2 * t - 3);
	float h_10 = t * (t - 1) * (t - 1);
	float h_11 = t * t * (t - 1);
	vec2 C_t= h_00 * cornerMotion.P0 + h_01 * cornerMotion.P1 + h_10 * cornerMotion.T0 + h_11 * cornerMotion.T1;

	//compute C'(t) and use this to change the angle based on the difference in velocity.
	float hprime_00 = 6 * (t - 1) * t;
	float hprime_01 = -6 * (t - 1) * t;
	float hprime_10 = 3*t*t - 4*t + 1;
	float hprime_11 = t*(3*t-2);

	vec2 Cprime_t = hprime_00 * cornerMotion.P0 + hprime_01 * cornerMotion.P1 + hprime_10 * cornerMotion.T0 + hprime_11 * cornerMotion.T1; //velocity at time t

	auto theta = atan2(-Cprime_t.y,Cprime_t.x); //works in all 4 quadrants
	auto angle_diff = theta - cornerMotion.theta_old;
	motion.angle -= angle_diff;
	cornerMotion.theta_old = theta;

	return C_t;
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
			auto backwardVelocity = vec2(projectileMotion.velocity.x * (-2*TileSystem::getScale()), projectileMotion.velocity.y * (-2 * TileSystem::getScale()));
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
	transform1.rotate(motion1.angle);
	transform1.scale(motion1.scale);
	for (int i = 0; i < vertices1.size(); i++)
	{
		vec3 position = vec3(vertices1[i].position.x, vertices1[i].position.y, 1.0);
		vertices1[i].position = transform1.mat * position;
	}
	Transform transform2;
	transform2.translate(motion2.position);
	transform2.rotate(motion2.angle);
	transform2.scale(motion2.scale);
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
			// edited this to fit SlugProjectile...
			if (ECS::registry<Projectile>.has(entity1))
			{
				//projectile is entity1, wall is entity2
				bounceProjectileOffWall(motion1, vertices1, vertices2);
			}
			else if (ECS::registry<SlugProjectile>.has(entity1)) {
				bounceProjectileOffWall(motion1, vertices1, vertices2);
			}
			else if (ECS::registry<SlugProjectile>.has(entity2)) {
				bounceProjectileOffWall(motion2, vertices2, vertices1);
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
					else if (ECS::registry<SlugProjectile>.has(entity1)) {
						bounceProjectileOffWall(motion1, vertices1, vertices2);
					}
					else if (ECS::registry<SlugProjectile>.has(entity2)) {
						bounceProjectileOffWall(motion2, vertices2, vertices1);
					}
					else
					{
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
		(ECS::registry<WallTile>.has(entity_i) && ECS::registry<Projectile>.has(entity_j)) ||
		(ECS::registry<SlugProjectile>.has(entity_i) && ECS::registry<WallTile>.has(entity_j)) ||
		(ECS::registry<WallTile>.has(entity_i) && ECS::registry<SlugProjectile>.has(entity_j));
}

//returns true if we have a possibility of caring if entity_i and entity_j collide
bool shouldCheckCollision(ECS::Entity entity_i, ECS::Entity entity_j)
{
	if (ECS::registry<DeathTimer>.has(entity_i) || ECS::registry<DeathTimer>.has(entity_j))
		return false;

	bool isValidSnailCollision_i = ECS::registry<Snail>.has(entity_i) &&
		(ECS::registry<Spider>.has(entity_j) || ECS::registry<WaterTile>.has(entity_j) || ECS::registry<SlugProjectile>.has(entity_j) ||
			ECS::registry<Slug>.has(entity_j) ||
                (!ECS::registry<NoCollide>.has(entity_j) && ECS::registry<Collectible>.has(entity_j)));

	bool isValidSnailCollision_j = ECS::registry<Snail>.has(entity_j) &&
		(ECS::registry<Spider>.has(entity_i) || ECS::registry<WaterTile>.has(entity_i) || ECS::registry<SlugProjectile>.has(entity_i) ||
			ECS::registry<Slug>.has(entity_i) ||
		        (!ECS::registry<NoCollide>.has(entity_i) && ECS::registry<Collectible>.has(entity_i)));

	bool isValidSnailProjectileCollision_i = ECS::registry<SnailProjectile>.has(entity_i) &&
		(ECS::registry<Spider>.has(entity_j) || ECS::registry<WallTile>.has(entity_j) || 
			ECS::registry<Slug>.has(entity_j) || ECS::registry<SlugProjectile>.has(entity_j));

	bool isValidSnailProjectileCollision_j = ECS::registry<SnailProjectile>.has(entity_j) &&
		(ECS::registry<Spider>.has(entity_i) || ECS::registry<WallTile>.has(entity_i) ||
			ECS::registry<Slug>.has(entity_i) || ECS::registry<SlugProjectile>.has(entity_i));

	bool isValidSlugProjectileCollision_i = ECS::registry<SlugProjectile>.has(entity_i) &&
		(ECS::registry<WallTile>.has(entity_j));

	bool isValidSlugProjectileCollision_j = ECS::registry<SlugProjectile>.has(entity_j) &&
		(ECS::registry<WallTile>.has(entity_i));
	

	return isValidSnailCollision_i || isValidSnailCollision_j || 
		isValidSnailProjectileCollision_i || isValidSnailProjectileCollision_j ||
		isValidSlugProjectileCollision_i || isValidSlugProjectileCollision_j;
}

void PhysicsSystem::stepToDestinationAroundCorner(ECS::Entity entity, float step_seconds) 
{
	auto& cornerMotion = ECS::registry<CornerMotion>.get(entity);
	if (!ECS::registry<Destination>.has(entity)) 
	{
		return; //don't move if it doesn't have a destination
	}
	auto& motion = ECS::registry<Motion>.get(entity);
	auto oldPosition = motion.position;
	auto& dest = ECS::registry<Destination>.get(entity);
	auto newPos = StepAroundCorner(entity, step_seconds, motion);
	if (cornerMotion.t >= 1)
	{
		//completed a segment
		cornerMotion.numSegmentsCompleted++;
		if (cornerMotion.numSegmentsCompleted == cornerMotion.numSegments)
		{
			// reached final destination
			motion.angle = cornerMotion.dest_angle;
			UpdateTileOccupancy(motion.position, dest.position);
			motion.position = dest.position;
			ECS::registry<Destination>.remove(entity);
			ECS::registry<CornerMotion>.remove(entity); //no longer rounding the corner
		}
		else 
		{
			UpdateTileOccupancy(motion.position, newPos);
			motion.position = newPos;
			SetupNextCornerSegment(entity, motion);
		}
	}
	else
	{
		UpdateTileOccupancy(motion.position, newPos);
		motion.position = newPos;
	}

	if (ECS::registry<Snail>.has(entity)) 
	{
		for (auto blurEntity : ECS::registry<BlurParticle>.entities)
		{
			vec2 velocity = motion.position - oldPosition;
			auto& motion2 = ECS::registry<Motion>.get(blurEntity);
			motion2.position += (velocity + motion2.velocity);
			float scaleFactor = 0.99f;
			motion2.scale *= scaleFactor;
			motion2.angle = motion.angle;
			motion2.lastDirection = motion.lastDirection;
		}
	}
}

void PhysicsSystem::stepToDestination(ECS::Entity entity, float step_seconds)
{
    
    auto& motion = ECS::registry<Motion>.get(entity);
    vec2 oldPosition = motion.position;
    auto& destReg = ECS::registry<Destination>;
    if (destReg.has(entity))
    {
		bool isSnailOrSpider = (ECS::registry<Snail>.has(entity) || ECS::registry<Spider>.has(entity));
        auto& dest = destReg.get(entity);
		vec2 newPos = motion.position + (motion.velocity * step_seconds);
		if ((dot(motion.position - newPos, dest.position - newPos) > 0) || (dest.position == newPos)) 
		{
			if (isSnailOrSpider)
			{
				UpdateTileOccupancy(motion.position, dest.position);
			}
			// overshot or perfectly hit destination
			// set velocity back to 0 stop moving
			motion.position = dest.position;
			//motion.velocity = { 0.f, 0.f };
			destReg.remove(entity);
		}
		else 
		{
			if (isSnailOrSpider)
			{
				UpdateTileOccupancy(motion.position, newPos);
			}
			motion.position = newPos;
		}
    }
    if(ECS::registry<Snail>.has(entity)) 
	{
        motion = ECS::registry<Motion>.get(entity);
        for (auto blurEntity : ECS::registry<BlurParticle>.entities)
        {
            vec2 velocity = motion.position - oldPosition;
            auto& motion2 = ECS::registry<Motion>.get(blurEntity);
            motion2.position += (velocity + motion2.velocity);
            float scaleFactor = 0.99f;
            motion2.scale *= scaleFactor;
            motion2.angle = motion.angle;
            motion2.lastDirection = motion.lastDirection;
        }
    }
}

bool areRoundingCorner(Motion& motion)
{
	return motion.velocity.x != 0 && motion.velocity.y != 0;
	// if it has non-zero x and y for velocity, then it's not vertical or horizontal, so we are rounding the corner
}

void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    (void)window_size_in_game_units;
    float step_seconds = 1.0f * (elapsed_ms / 1000.f);
    WeatherParentParticle::nextSpawn -= elapsed_ms;
    WeatherParticle::nextSpawn -= elapsed_ms;
    bool areAllDeprecated = true;

    for (auto entity : ECS::registry<WeatherParentParticle>.entities) {
        if(!ECS::registry<Deprecated>.has(entity)) {
            areAllDeprecated = false;
        }
    }
    if (ECS::registry<WeatherParentParticle>.components.size() < WeatherParentParticle::count && WeatherParentParticle::nextSpawn < 0 && areAllDeprecated)
    {
        
        ECS::Entity newSnowflakeParticle;
        Particle::createWeatherParticle("snowflake", newSnowflakeParticle, window_size_in_game_units);
        WeatherParentParticle::nextSpawn = Particle::timer * uniform_dist(rng);
    }
    
    auto& camera = ECS::registry<Camera>.entities[0];
    vec2 cameraOffset = ECS::registry<Motion>.get(camera).position;

    for (auto entity : ECS::registry<WeatherParentParticle>.entities) {
        bool isDeprecated = ECS::registry<Deprecated>.has(entity);
        auto& element = ECS::registry<WeatherParentParticle>.get(entity);
        int particlesSize = element.particles.size();
        if(isDeprecated) {
            if(particlesSize == 0)
                ECS::ContainerInterface::remove_all_components_of(entity);
        } else {
            if(particlesSize < WeatherParticle::count && WeatherParticle::nextSpawn < 0)
            {
                ECS::Entity newSnowflakeParticle;
                Particle::createWeatherChildParticle("snowflake", newSnowflakeParticle, entity, window_size_in_game_units);
                element.particles.push_back(newSnowflakeParticle);
                WeatherParticle::nextSpawn = Particle::timer * uniform_dist(rng);
            }
        }
    }
    
    int i = 0;
    for (auto entity : ECS::registry<WeatherParticle>.entities)
    {
        auto& parentEntity = ECS::registry<WeatherParticle>.get(entity).parentEntity;
        
        auto& motion = ECS::registry<Motion>.get(entity);
        DeathTimer& dt = ECS::registry<DeathTimer>.get(entity);
        bool isDeprecated = ECS::registry<Deprecated>.has(parentEntity);
        
        if(!ECS::registry<WeatherParentParticle>.has(entity)) {
            
            int phaseTime = WeatherParentParticle::timer/5;
            
            RejectedStages& rs = ECS::registry<RejectedStages>.get(entity);
            if(!rs.rejectedState3 && (dt.counter_ms <= WeatherParentParticle::timer-(2*phaseTime)) ) {
                if(randomBool()){
                    Particle::setP3Motion(motion);
                } else {
                    rs.rejectedState3 = true;
                }
            } else if(!rs.rejectedState2 && (dt.counter_ms <= WeatherParentParticle::timer-(phaseTime))) {
                if(randomBool()){
                    Particle::setP2Motion(motion);
                } else {
                    rs.rejectedState2 = true;
                }
            }
            
            if(isDeprecated && WorldSystem::offScreen(motion.position, window_size_in_game_units, cameraOffset)) {
                auto& elementList = ECS::registry<WeatherParentParticle>.get(parentEntity).particles;
                
                auto it = elementList.begin();
                while (it != elementList.end())
                {
                    if (it->id == entity.id) {
                        it = elementList.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                ECS::ContainerInterface::remove_all_components_of(entity);
                continue;
            } else if(!isDeprecated && WorldSystem::offScreenExceptNegativeYWithBuffer(motion.position, window_size_in_game_units, cameraOffset, 250)) {
                    float xValue = 0;
                    int minimum_number = 0;
                    int max_number = 0;
                    float yValue = 0;
                    minimum_number = cameraOffset.x - 10;
                    max_number = cameraOffset.x + window_size_in_game_units.x + 200;
                    xValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
                    minimum_number = cameraOffset.y - 100;
                    max_number = cameraOffset.y - 1;
                    yValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
                    Particle::setP1Motion(motion);
                    motion.position = { xValue , yValue};
                    dt.counter_ms = WeatherParentParticle::timer;
                    RejectedStages& rs = ECS::registry<RejectedStages>.get(entity);
                    rs.rejectedState2 = false;
                    rs.rejectedState3 = false;
            }
            
            float gravity_acceleration = 0.0004;
            motion.velocity += gravity_acceleration * step_seconds;
            motion.position += motion.velocity * step_seconds;
            i++;
        }
    }
    
    //now we update the state of the entities with regards to which entites are rounding the corner
    for (auto entity : ECS::registry<Destination>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(entity);
        if (!ECS::registry<CornerMotion>.has(entity) && areRoundingCorner(motion))
        {
            //then initialize rounding the corner
            auto& dest = ECS::registry<Destination>.get(entity);
            SetupCornerMovement(entity, dest);
            SetupNextCornerSegment(entity, motion);
        }
    }
    // Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
    if (turnType == PLAYER_WAITING)
    {
        // Projectile previews
        for (auto entity : ECS::registry<SnailProjectile::Preview>.entities)
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
		if (ECS::registry<Destination>.has(snailEntity)) //don't want to do any stepping if we don't have a destination
		{
			if (ECS::registry<CornerMotion>.has(snailEntity))
			{
				stepToDestinationAroundCorner(snailEntity, step_seconds);
			}
			else
			{
				stepToDestination(snailEntity, step_seconds);
			}
		}
		ECS::Entity entity;
        auto& motion = ECS::registry<Motion>.get(snailEntity);
        Particle::createParticle(motion, entity);
    }
	else if (turnType == ENEMY)
    {
        // move the projectile!
	    // There shouldn't be any previews at this point, but to be safe
		SnailProjectile::Preview::removeCurrent();
        for (auto entity : ECS::registry<SnailProjectile>.entities)
        {
            auto& motion = ECS::registry<Motion>.get(entity);
            vec2 velocity = motion.velocity;
            motion.position += velocity * step_seconds;
        }
		// making sure slug projectiles move
		for (auto entity : ECS::registry<SlugProjectile>.entities)
		{
			auto& motion = ECS::registry<Motion>.get(entity);
			vec2 velocity = motion.velocity;
			motion.position += velocity * step_seconds;
		}

        // move enemies
        for (auto entity : ECS::registry<Enemy>.entities)
        {
			if (ECS::registry<CornerMotion>.has(entity))
			{
				stepToDestinationAroundCorner(entity, step_seconds);
			}
			else
			{
				stepToDestination(entity, step_seconds);
			}
        }
    }
	else if (turnType == CAMERA)
    {
        // move camera
        auto &cameraEntity = ECS::registry<Camera>.entities[0];
        stepToDestination(cameraEntity, step_seconds);
        for (auto entity : ECS::registry<WeatherParentParticle>.entities) {
            auto& element = ECS::registry<WeatherParentParticle>.get(entity);
            int particlesSize = element.particles.size();

            if(!ECS::registry<Deprecated>.has(entity) && particlesSize != 0)
                ECS::registry<Deprecated>.emplace(entity);
        }
    }

	Equipped::moveEquippedWithHost();

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
      if (shouldCheckCollision(entity_i, entity_j))
			{
				bool doPenetrationFree = isProjectileAndWall(entity_i, entity_j);
				if (collides(entity_i, entity_j, motion_i, motion_j, doPenetrationFree))
				{
                    if((entity_i.id != WaterTile::splashEntityID && entity_j.id != WaterTile::splashEntityID) && (ECS::registry<WaterTile>.has(entity_i) || ECS::registry<WaterTile>.has(entity_j))) {
                        ECS::Entity e = ECS::registry<WaterTile>.has(entity_i) ? entity_i : entity_j;
                        notify(Event(Event::SPLASH, e));
                    } else {
                        notify(Event(Event::COLLISION, entity_i, entity_j));
                        notify(Event(Event::COLLISION, entity_j, entity_i));
                        //notify both entities of collision
                        notify(Event(Event::COLLISION, entity_i, entity_j));
                        notify(Event(Event::COLLISION, entity_j, entity_i));
                    }
                    
				}
			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}
