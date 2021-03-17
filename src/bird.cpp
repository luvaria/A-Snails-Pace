// Header
#include "bird.hpp"
#include "render.hpp"
#include "ai.hpp"
#include "tiles/tiles.hpp"

ECS::Entity Bird::createBird(vec2 position, ECS::Entity entity)
{
	// Create rendering primitives
	std::string key = "bird";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("bird.obj"));
		RenderSystem::createColoredMesh(resource, "bird");
	}

	
	std::string key_min = "minBird";
	ShadedMesh& resource_min = cache_resource(key_min);
	if (resource_min.mesh.vertices.size() == 0)
	{
		resource_min.mesh.loadFromMinOBJFile(mesh_path("minbird.obj"));
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
	ECS::registry<Bird>.emplace(entity);

	// Adding Behaviour Tree to Bird

	//std::shared_ptr <BTNode> isr = std::make_unique<IsSnailInRange>();
	//std::shared_ptr <BTNode> gwt = std::make_unique<GetWithinTwoTiles>();
	//std::shared_ptr <BTNode> rfn = std::make_unique<RepeatForN>(gwt, 5);
	//std::shared_ptr <BTNode> fxs = std::make_unique<FireXShots>(2);
	//std::shared_ptr <BTNode> selector = std::make_unique<BTSelector>(std::vector<std::shared_ptr <BTNode>>({ rfn, fxs }));
	//std::shared_ptr <BTNode> fxs2 = std::make_unique<FireXShots>(2);
	//std::shared_ptr <BTNode> ps = std::make_unique<PredictShot>();
	//std::shared_ptr <BTNode> randomSelector = std::make_unique<RandomSelector>(75, fxs, ps);
	//std::shared_ptr <BTNode> gts = std::make_unique<GetToSnail>();


	// testing to see if there is some issue with ai.step
	std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>();
	std::shared_ptr <BTNode> iSr = std::make_unique<IsSnailInRange>();
	std::shared_ptr <BTNode> rfn = std::make_unique<RepeatForN>(lfs, 100);
	std::shared_ptr <BTNode> bsn = std::make_unique<BTSequenceForN>(std::vector<std::shared_ptr <BTNode>>({ lfs }));
	std::shared_ptr <BTNode> fxs = std::make_unique<FireXShots>(2);
	//std::shared_ptr <BTNode> selector = std::make_unique<BTSelector>(std::vector<std::shared_ptr <BTNode>>({ rfn, fxs }));
	std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ iSr, rfn }));

	//std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ isr, selector, randomSelector, gts }));
	//std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ gwt }));



	tree->init(entity);
	auto& ai = ECS::registry<AI>.get(entity);
	ai.tree = tree;
	//auto& ai = ECS::registry<AI>.get(entity);
	//ai.tree = tree;
	//ECS::registry<std::shared_ptr <BTNode>>.emplace(entity);
	return entity;
}