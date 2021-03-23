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
    //auto& ai = ECS::registry<AI>.get(entity);
    //ai.tree = tree;
    tree->init(entity);
    auto& ai = ECS::registry<AI>.get(entity);
    ai.tree = tree;
    //ECS::registry<std::shared_ptr <BTNode>>.emplace(entity);
    return entity;
    
}

ECS::Entity Spider::createSpider(vec2 position, ECS::Entity entity)
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

	// Initialize the motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f }; // 200
	motion.position = position;
	motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
	motion.scale.y *= -1; // fix orientation
	motion.scale *= 0.9f;
	motion.lastDirection = DIRECTION_WEST;

	ECS::registry<AI>.emplace(entity);
	ECS::registry<Enemy>.emplace(entity);
	ECS::registry<Spider>.emplace(entity);
	ECS::registry<DirectionInput>.emplace(entity);

	
	// Adding Behaviour Tree to Spider
	// Maybe add some registry that keeps track of trees??
	std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>(false);
	std::shared_ptr <BTNode> isr = std::make_unique<IsSnailInRange>();
	std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ isr, lfs }));
	//auto& ai = ECS::registry<AI>.get(entity);
	//ai.tree = tree;

	tree->init(entity);
	auto& ai = ECS::registry<AI>.get(entity);
	ai.tree = tree;
	//ECS::registry<std::shared_ptr <BTNode>>.emplace(entity);
	return entity;
}
