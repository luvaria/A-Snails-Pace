// Header
#include "projectile.hpp"
#include "render.hpp"
#include "world.hpp"

std::chrono::time_point<std::chrono::high_resolution_clock> Projectile::Preview::s_can_show_projectile_preview_time
    = std::chrono::time_point<std::chrono::high_resolution_clock>::max();
vec2 Projectile::Preview::s_projectile_preview_pos = {0.f,0.f};

ECS::Entity Projectile::createProjectile(vec2 position, vec2 velocity, bool preview /* = false */)
{
	auto entity = ECS::Entity();

	// Create rendering primitives
	std::string key = preview ? "projectile_preview" : "projectile";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("shell.obj"));
		RenderSystem::createColoredMesh(resource, "projectile");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

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
	if (preview)
    {
	    ECS::registry<Preview>.emplace(entity);
        resource.texture.alpha = 0.25f;
        motion.scale *= 0.5f;
    }

	return entity;
}


