// Header
#include "vine.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity VineTile::createVineTile(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "vine";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("vine.obj"));
		RenderSystem::createColoredMesh(resource, "tile");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { TileSystem::getScale(), -TileSystem::getScale() };

	// Create an (empty) VineTile component
	ECS::registry<VineTile>.emplace(entity);

	return entity;
}
