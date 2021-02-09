// Header
#include "snail.hpp"
#include "render.hpp"

ECS::Entity Snail::createSnail(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "snail";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("snail.obj"));
		RenderSystem::createColoredMesh(resource, "snail");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = resource.mesh.original_size * 35.f;
	motion.scale.y *= -1; // fix orientation

	// Create an (empty) Snail component
	ECS::registry<Snail>.emplace(entity);

	return entity;
}
