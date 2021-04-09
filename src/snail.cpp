// Header
#include "snail.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "common.hpp"

ECS::Entity Snail::createSnail(vec2 position, ECS::Entity entity)
{
	Motion motion = Motion();
	motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.lastDirection = DIRECTION_EAST;

    createSnail(motion, entity);

    std::string key = "snail";
    ShadedMesh& resource = cache_resource(key);
    Motion& updatedMotion = ECS::registry<Motion>.get(entity);
    updatedMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    updatedMotion.scale.y *= -1; // fix orientation
    updatedMotion.scale *= 0.9f;

	return entity;
}

ECS::Entity Snail::createSnail(Motion motion, ECS::Entity entity)
{
    std::string key = "snail";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("snail.obj"));
        RenderSystem::createColoredMesh(resource, "snail");
    }

    std::string key_min = "snail-min";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("snail-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::PLAYER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::PLAYER);

    // Setting initial motion values
    ECS::registry<Motion>.insert(entity, std::move(motion));

    // Create an (empty) Snail component
    ECS::registry<Snail>.emplace(entity);
    ECS::registry<DirectionInput>.emplace(entity);

    return entity;
}
