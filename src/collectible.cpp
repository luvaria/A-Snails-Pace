#include "collectible.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "snail.hpp"
#include "npc.hpp"

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
        RenderSystem::createColoredMesh(resource, "colored_mesh");
    }

    std::string key_min = "min" + name;
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path(name + "-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::COLLECTIBLE);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::COLLECTIBLE);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    motion.scale.y *= -1; // fix orientation
    motion.scale *= 0.8f;
    motion.lastDirection = DIRECTION_WEST;

    // Create a Collectible component
    auto& collectible = ECS::registry<Collectible>.emplace(entity);
    collectible.id = id;

    return entity;
}

void Collectible::equip(ECS::Entity host, int id)
{
    assert(id >= 0 && id < collectibleMap.size());

    unequip(host);

    if (ECS::registry<Player>.has(host))
        ECS::registry<Inventory>.components[0].equipped = id;

    auto& motion = ECS::registry<Motion>.get(host);
    auto collectEntity = createCollectible(motion.position, id);
    ECS::registry<NoCollide>.emplace(collectEntity);

    // npc hats don't layer over snail hat
    if (!ECS::registry<Player>.has(host))
    {
        auto& collectMeshRef = ECS::registry<ShadedMeshRef>.get(collectEntity);
        collectMeshRef.renderBucket = RenderBucket::CHARACTER_COLLECTIBLE;
    }

    ECS::registry<Equipped>.emplace(host, collectEntity);
}

void Collectible::unequip(ECS::Entity host)
{
    if (ECS::registry<Player>.has(host))
        ECS::registry<Inventory>.components[0].equipped = -1;

    if (ECS::registry<Equipped>.has(host))
    {
        ECS::Entity collectible = ECS::registry<Equipped>.get(host).collectible;
        ECS::ContainerInterface::remove_all_components_of(collectible);
        ECS::registry<Equipped>.remove(host);
    }
}

void Equipped::moveEquippedWithHost()
{
    for (size_t i = 0; i < ECS::registry<Equipped>.size(); i++)
    {
        ECS::Entity& hostEntity = ECS::registry<Equipped>.entities[i];
        ECS::Entity& collectEntity = ECS::registry<Equipped>.components[i].collectible;
        auto& hostMotion = ECS::registry<Motion>.get(hostEntity);
        auto& collectMotion = ECS::registry<Motion>.get(collectEntity);
        collectMotion.angle = hostMotion.angle;

        const int HOST_SCALE_X_SIGN = hostMotion.scale.x / abs(hostMotion.scale.x);
        const int HOST_SCALE_Y_SIGN = hostMotion.scale.y / abs(hostMotion.scale.y);

        // match scale signs (flip)
        collectMotion.scale.x = abs(collectMotion.scale.x) * HOST_SCALE_X_SIGN;
        collectMotion.scale.y = abs(collectMotion.scale.y) * HOST_SCALE_Y_SIGN;

        collectMotion.position = hostMotion.position;
        const float COLLECTIBLE_SHIFT = 0.5f * abs(collectMotion.scale.y);
        //shift pos above snail based on the angle 
        vec2 unit_shift = vec2(cos(-collectMotion.angle + PI/2), -sin(-collectMotion.angle + PI/2));
        collectMotion.position += COLLECTIBLE_SHIFT * unit_shift;

    }
}
