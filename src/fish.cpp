// Header
#include "fish.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Fish::createFish(vec2 position, ECS::Entity entity)
{
	// Create rendering primitives
	std::string key = "fish";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("fish.obj"));
		RenderSystem::createColoredMesh(resource, "fish");
	}

	std::string key_min = "min-fish";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("fish-min.obj"));
	}


	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
	ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

	// Initialize the motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f }; // 200
	motion.position = position;
	motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
	motion.scale.y *= -1; // fix orientation
	motion.scale *= 0.3f;
	motion.lastDirection = DIRECTION_NORTH;
	auto& move = ECS::registry<Move>.emplace(entity);
	move.direction = true;
	move.hasMoved = true;

	
	ECS::registry<Enemy>.emplace(entity);
	ECS::registry<Invincible>.emplace(entity);
	ECS::registry<Fish>.emplace(entity);
	ECS::registry<DirectionInput>.emplace(entity);

	return entity;
}
