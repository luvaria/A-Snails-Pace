// Header
#include "ground.hpp"
#include "render.hpp"
#include "world.hpp"

ECS::Entity GroundTile::createGroundTile(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "ground";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("ground.obj"));
		RenderSystem::createColoredMesh(resource, "tile");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { WorldSystem::getScale(), -WorldSystem::getScale() };

	// Create an (empty) GroundTile component
	ECS::registry<GroundTile>.emplace(entity);

	return entity;
}
