// Header
#include "bird.hpp"
#include "render.hpp"
#include "ai.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Bird::createBird(vec2 position, ECS::Entity entity)
{
	// Create rendering primitives
	std::string key = "bird";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("bird.obj"));
		RenderSystem::createColoredMesh(resource, "bird");
	}

	
	std::string key_min = "bird-min";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("bird-min.obj"));
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
	motion.scale *= 0.9f;
	motion.lastDirection = DIRECTION_WEST;

	auto& fire = ECS::registry<Fire>.emplace(entity);
	fire.fired = false;

	//ECS::registry<AI>.emplace(entity);
	ECS::registry<Bird>.emplace(entity);
	ECS::registry<Enemy>.emplace(entity);
	ECS::registry<Invincible>.emplace(entity);
	ECS::registry<DirectionInput>.emplace(entity);

	return entity;
}