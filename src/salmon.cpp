// Header
#include "salmon.hpp"
#include "render.hpp"

ECS::Entity Salmon::createSalmon(vec2 position)
{
	auto entity = ECS::Entity();

	std::string key = "salmon";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("salmon.obj"));
		RenderSystem::createColoredMesh(resource, "salmon");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = resource.mesh.original_size * 150.f;
	motion.scale.x *= -1; // point front to the right

	// Create and (empty) Salmon component to be able to refer to all turtles
	ECS::registry<Salmon>.emplace(entity);

	return entity;
}
