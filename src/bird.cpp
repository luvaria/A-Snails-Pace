// Header
#include "bird.hpp"
#include "render.hpp"
#include "ai.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Bird::createBird(vec2 position, ECS::Entity entity)
{
    // Initialize the motion
    auto motion = Motion();
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f }; // 200
    motion.position = position;
    motion.lastDirection = DIRECTION_WEST;

    createBird(motion, entity);

    std::string key = "bird";
    ShadedMesh& resource = cache_resource(key);
    Motion& updatedMotion = ECS::registry<Motion>.get(entity);
    updatedMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    updatedMotion.scale.y *= -1; // fix orientation
    updatedMotion.scale *= 0.9f;

    return entity;
}

ECS::Entity Bird::createBird(Motion motion, ECS::Entity entity)
{
    // Create rendering primitives
    std::string key = "bird";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("bird.obj"));
        RenderSystem::createColoredMesh(resource, "bird");
    }


    std::string key_min = "bird-min";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("bird-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    ECS::registry<Motion>.insert(entity, std::move(motion));

    auto& fire = ECS::registry<Fire>.emplace(entity);
    fire.fired = false;

    ECS::registry<Bird>.emplace(entity);
    ECS::registry<Enemy>.emplace(entity);
    ECS::registry<DirectionInput>.emplace(entity);

    return entity;
}