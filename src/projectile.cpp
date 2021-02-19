// Header
#include "projectile.hpp"
#include "render.hpp"
#include "world.hpp"

ECS::Entity Projectile::createProjectile(vec2 position, vec2 velocity)
{
	auto entity = ECS::Entity();

	// Create rendering primitives
	std::string key = "projectile";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("shell.obj"));
		RenderSystem::createColoredMesh(resource, "projectile");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::PROJECTILE);

	// Initialize the motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.position = position;
	motion.velocity = velocity;
	//divided by 5 since it shouldn't take up the whole tile
	motion.scale = (resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale()) / 5.0f;
	motion.scale.y *= -1; // fix orientation

	// Create and (empty) Projectile component to be able to refer to all projectiles
	ECS::registry<Projectile>.emplace(entity);

	return entity;
}
