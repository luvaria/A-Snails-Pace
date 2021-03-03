// Header
#include "snail.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "common.hpp"

ECS::Entity Snail::createSnail(vec2 position, ECS::Entity entity)
{
	std::string key = "snail";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("snail.obj"));
		RenderSystem::createColoredMesh(resource, "snail");
	}

	std::string key_min = "minSnail";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("snail-min.obj"));
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
	ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
    motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
	motion.scale.y *= -1; // fix orientation
	motion.scale *= 0.9f;
    motion.lastDirection = DIRECTION_EAST;

	// Create an (empty) Snail component
	ECS::registry<Snail>.emplace(entity);

	return entity;
}
