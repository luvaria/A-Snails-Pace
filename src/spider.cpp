// Header
#include "spider.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Spider::createSpider(vec2 position)
{
	auto entity = ECS::Entity();

	// Create rendering primitives
	std::string key = "spider";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("spider.obj"));
		RenderSystem::createColoredMesh(resource, "spider");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);

	// Initialize the motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f }; // 200
	motion.position = position;
    motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
	motion.scale.y *= -1; // fix orientation
    motion.lastDirection = DIRECTION_WEST;

	// Create and (empty) Spider component to be able to refer to all spiders
	ECS::registry<Spider>.emplace(entity);

	return entity;
}
