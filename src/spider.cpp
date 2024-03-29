// Header
#include "spider.hpp"
#include "render.hpp"
#include "ai.hpp"
#include "tiles/tiles.hpp"
#include "particle.hpp"

/* Credits:
https://learnopengl.com/Advanced-OpenGL/Geometry-Shader#:~:text=Using%20the%20vertex%20data%20from,generate%20one%20line%20strip%20primitive.
*/
ECS::Entity Spider::createExplodingSpider(Motion givenMotion, ECS::Entity entity)
{

    // Create rendering primitives
    std::string key = "explodingSpider";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("spider.obj"));
        RenderSystem::createColoredMesh(resource, "exploding");
    }

    std::string key_min = "explodingMinSpider";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("spider-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    // Initialize the motion
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = givenMotion.position;
    motion.angle = givenMotion.angle;
    motion.scale = givenMotion.scale;
    ECS::registry<AI>.emplace(entity);
    ECS::registry<Spider>.emplace(entity);
	ECS::registry<DirectionInput>.emplace(entity);
    DeathTimer& dt = ECS::registry<DeathTimer>.emplace(entity);
    dt.counter_ms = Particle::timer*10;
    // Adding Behaviour Tree to Spider
    // Maybe add some registry that keeps track of trees??
    std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>();
    std::shared_ptr <BTNode> isr = std::make_unique<IsSnailInRange>();
    std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ isr, lfs }));
    tree->init(entity);
    auto& ai = ECS::registry<AI>.get(entity);
    ai.tree = tree;
    return entity;
}

ECS::Entity Spider::createSpider(vec2 position, ECS::Entity entity)
{
    // Initialize the motion
    auto motion = Motion();
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f }; // 200
    motion.position = position;
    motion.lastDirection = DIRECTION_WEST;

    createSpider(motion, entity);

    std::string key = "spider";
    ShadedMesh& resource = cache_resource(key);
    Motion& updatedMotion = ECS::registry<Motion>.get(entity);
    updatedMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    updatedMotion.scale.y *= -1; // fix orientation
    updatedMotion.scale *= 0.9f;

    return entity;
}

ECS::Entity Spider::createSpider(Motion motion, ECS::Entity entity, std::shared_ptr<BTNode> tree /* = nullptr */)
{
    // Create rendering primitives
    std::string key = "spider";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("spider.obj"));
        RenderSystem::createColoredMesh(resource, "spider");
    }

    std::string key_min = "min-spider";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("spider-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    ECS::registry<Motion>.insert(entity, std::move(motion));

    ECS::registry<AI>.emplace(entity);
    ECS::registry<Enemy>.emplace(entity);
    ECS::registry<Spider>.emplace(entity);
    ECS::registry<DirectionInput>.emplace(entity);


    // Adding Behaviour Tree to Spider
    // Maybe add some registry that keeps track of trees??
    if (!tree)
    {
        std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>(false);
        std::shared_ptr <BTNode> isr = std::make_unique<IsSnailInRange>();
        tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ isr, lfs }));
        tree->init(entity);
    }

    auto& ai = ECS::registry<AI>.get(entity);
    ai.tree = tree;
    return entity;
}


ECS::Entity SuperSpider::createSuperSpider(vec2 position, ECS::Entity entity)
{
    // Initialize the motion
    auto motion = Motion();
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f }; // 200
    motion.position = position;
    motion.lastDirection = DIRECTION_WEST;

    createSuperSpider(motion, entity);

    std::string key = "superspider";
    ShadedMesh& resource = cache_resource(key);
    Motion& updatedMotion = ECS::registry<Motion>.get(entity);
    updatedMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
    updatedMotion.scale.y *= -1; // fix orientation
    // was originally 0.9f, trying to make superspider slightly bigger than normal spider
    updatedMotion.scale *= 1.1f;

    return entity;
}

ECS::Entity SuperSpider::createSuperSpider(Motion motion, ECS::Entity entity, std::shared_ptr<BTNode> tree)
{
    // Create rendering primitives
    std::string key = "superspider";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("superspider.obj"));
        RenderSystem::createColoredMesh(resource, "superspider");
    }

    std::string key_min = "min-superspider";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("superspider-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::CHARACTER);

    ECS::registry<Motion>.insert(entity, std::move(motion));

    ECS::registry<AI>.emplace(entity);
    ECS::registry<Enemy>.emplace(entity);
    ECS::registry<SuperSpider>.emplace(entity);
    ECS::registry<DirectionInput>.emplace(entity);

    auto& fire = ECS::registry<Fire>.emplace(entity);
    fire.fired = false;

    // Adding Behaviour Tree to super spider
    if (!tree)
    {
        std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>(false);
        std::shared_ptr <BTNode> isr = std::make_unique<IsSnailInRange>();
        tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ isr, lfs }));
        tree->init(entity);
    }

    auto& ai = ECS::registry<AI>.get(entity);
    ai.tree = tree;
    return entity;
}
