// Header
#include "fish.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Fish::createFish(vec2 position, ECS::Entity entity)
{
    // Initialize the motion
    auto motion = Motion();
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f }; // 200
    motion.position = position;
    motion.lastDirection = DIRECTION_NORTH;

    createFish(motion, entity);

    std::string key = "fish";
    ShadedMesh& resource = cache_resource(key);
    Motion& updatedMotion = ECS::registry<Motion>.get(entity);
    updatedMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    updatedMotion.scale.y *= -1; // fix orientation
    updatedMotion.scale *= 0.3f;

    return entity;
}

ECS::Entity Fish::createFish(Motion motion, ECS::Entity entity)
{
    // Create rendering primitives
    std::string key = "fish";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("fish.obj"));
        RenderSystem::createColoredMesh(resource, "fish");
    }

    std::string key_min = "min-fish";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("fish-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    ECS::registry<Motion>.insert(entity, std::move(motion));

    auto& move = ECS::registry<Move>.emplace(entity);
    move.direction = true;
    move.hasMoved = true;

    ECS::registry<Enemy>.emplace(entity);
    ECS::registry<Invincible>.emplace(entity);
    ECS::registry<Fish>.emplace(entity);
    ECS::registry<DirectionInput>.emplace(entity);

    return entity;
}

void Fish::Move::setFromJson(const json &saved)
{
    direction = saved[LoadSaveSystem::FISH_MOVE_DIRECTION_KEY];
    hasMoved = saved[LoadSaveSystem::FISH_MOVE_MOVED_KEY];
}

void Fish::Move::writeToJson(json &toSave)
{
    toSave[LoadSaveSystem::FISH_MOVE_DIRECTION_KEY] = direction;
    toSave[LoadSaveSystem::FISH_MOVE_MOVED_KEY] = hasMoved;
}
