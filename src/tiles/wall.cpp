// Header
#include "wall.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity WallTile::createWallTile(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "wall";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("wall.obj"));
		RenderSystem::createColoredMesh(resource, "tile");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::TILE);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { TileSystem::getScale(), -TileSystem::getScale() };

	// Create an (empty) WallTile component
	ECS::registry<WallTile>.emplace(entity);

	return entity;
}
