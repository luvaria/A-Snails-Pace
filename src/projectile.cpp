// Header
#include "projectile.hpp"
#include "render.hpp"
#include "world.hpp"

ECS::Entity SnailProjectile::createProjectile(vec2 position, vec2 velocity, bool preview /* = false */)
{
	auto entity = ECS::Entity();

	// Create rendering primitives
	std::string key = preview ? "projectile_preview" : "projectile";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("shell.obj"));
		RenderSystem::createColoredMesh(resource, "snail_projectile");
	}

	std::string key_min = "projectile-min";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("shell-min.obj"));
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::PROJECTILE);
	ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::PROJECTILE);

	// Initialize the motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.position = position;
	motion.velocity = velocity;
	//divided by 5 since it shouldn't take up the whole tile
	motion.scale = (resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale()) / 5.0f;
	motion.scale.y *= -1; // fix orientation

	ECS::registry<Projectile>.emplace(entity);
	ECS::registry<SnailProjectile>.emplace(entity);

	if (preview)
    {
	    ECS::registry<Preview>.emplace(entity);
        resource.texture.alpha = 0.5f;
        motion.velocity *= 12.f;
    }

	return entity;
}

void SnailProjectile::Preview::removeCurrent()
{
    for (auto& entity : ECS::registry<SnailProjectile::Preview>.entities)
    {
        ECS::ContainerInterface::remove_all_components_of(entity);
    }
}

// I've implemented it as a separate function so the collisions are easier to track
ECS::Entity SlugProjectile::createProjectile(vec2 position, vec2 velocity) {
	auto entity = ECS::Entity();

	// Create rendering primitives
	std::string key = "slug_projectile";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("slug_projectile.obj"));
		RenderSystem::createColoredMesh(resource, "slug_projectile");
	}

	std::string key_min = "slug_projectile-min";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("slug_projectile-min.obj"));
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::PROJECTILE);
	ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::PROJECTILE);

	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.position = position;
	motion.velocity = velocity;
	//divided by 5 since it shouldn't take up the whole tile
	motion.scale = (resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale()) / 5.0f;
	motion.scale.y *= -1; // fix orientation

	ECS::registry<Projectile>.emplace(entity);
	ECS::registry<SlugProjectile>.emplace(entity);
	

	return entity;
}



