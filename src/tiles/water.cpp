// Header
#include "water.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity WaterTile::createWaterTile(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "water";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("water.obj"));
		RenderSystem::createColoredMesh(resource, "tile");
	}

	std::string key_min = "minWater";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("water-min.obj"));
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::TILE);
	ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::TILE);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { TileSystem::getScale(), -TileSystem::getScale() };

	// Create an (empty) WaterTile component
	ECS::registry<WaterTile>.emplace(entity);

	return entity;
}
