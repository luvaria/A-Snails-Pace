#include "collectible.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "snail.hpp"

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
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::FOREGROUND);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::FOREGROUND);

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
    auto& motion = ECS::registry<Motion>.get(host);
    auto collectEntity = createCollectible(motion.position, id);
    if (ECS::registry<Equipped>.has(host))
    {
        // remove current equipped
        ECS::registry<Equipped>.remove(host);
    }
    ECS::registry<Equipped>.emplace(host, collectEntity);
}

void Equipped::moveEquippedWithHost()
{
    for (size_t i = 0; i < ECS::registry<Equipped>.size(); i++)
    {
        // TODO #54: get it to sit nicely on top of snail
        ECS::Entity& hostEntity = ECS::registry<Equipped>.entities[i];
        ECS::Entity& collectEntity = ECS::registry<Equipped>.components[i].collectible;
        auto& hostMotion = ECS::registry<Motion>.get(hostEntity);
        auto& collectMotion = ECS::registry<Motion>.get(collectEntity);
        collectMotion.position = hostMotion.position;
        collectMotion.angle = hostMotion.angle;
    }
}

bool Equipped::isEquipped(ECS::Entity collectible)
{
    for (auto& equipped : ECS::registry<Equipped>.components)
    {
        ECS::Entity equippedEntity = equipped.collectible;
        if (equippedEntity.id == collectible.id)
            return true;
    }
    return false;
}