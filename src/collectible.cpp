#include "collectible.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Collectible::createCollectible(vec2 position, int id)
{
    auto collectIt = collectibleMap.find(id);
    if (collectIt == collectibleMap.end())
    {
        throw std::runtime_error("Failed to map id " + std::to_string(id) + " to collectible map");
    }
    std::string const name = collectIt->second;

    ECS::Entity entity = ECS::Entity();

    std::string key = name + "_collect";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path(name +".obj"));
        RenderSystem::createColoredMesh(resource, name);
    }

    std::string key_min = "min" + name;
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path(name + "-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    motion.scale.y *= -1; // fix orientation
    motion.scale *= 0.5f;
    motion.lastDirection = DIRECTION_EAST;

    // Create a Collectible component
    auto& collectible = ECS::registry<Collectible>.emplace(entity);
    collectible.id = id;

    return entity;
}

void Collectible::equip(ECS::Entity host, int id)
{
    // TODO #54: implement this
    return;
}