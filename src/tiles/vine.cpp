// Header
#include "vine.hpp"
#include "render.hpp"

ECS::Entity VineTile::createVineTile(Tile& tile, ECS::Entity entity)
{
	ECS::registry<Tile>.insert(entity, std::move(tile));
	auto vineEntity = createVineTile(vec2(tile.x, tile.y), entity);
	tile.addObserver(&ECS::registry<VineTile>.get(vineEntity));
	return vineEntity;
}

ECS::Entity VineTile::createVineTile(vec2 position, ECS::Entity entity)
{
	std::string key = "vine";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		float numFrames = 6.0f;
		float numAnimations = 2.0f;
		resource.texture.frameSize = { 1.0f / numFrames, 1.0f / numAnimations }; // FRAME SIZE HERE!!! this is the percentage of the whole thing...
		RenderSystem::createSprite(resource, textures_path("vine.png"), "spriteSheet", true);
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::TILE);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { TileSystem::getScale(), TileSystem::getScale() };

	//setting the animation information
	SpriteSheet& spriteSheet = ECS::registry<SpriteSheet>.emplace(entity);
	spriteSheet.animationSpeed = 100;
	spriteSheet.numAnimationFrames = 6;
	spriteSheet.currentAnimationNumber = 1; //leaves move when the level starts.


	// Create an (empty) VineTile component
	auto& vineTile = ECS::registry<VineTile>.emplace(entity);
	vineTile.entity = entity;
	return entity;
}

void VineTile::onNotify(Event env) 
{
	if (env.type == Event::TILE_OCCUPIED) 
	{
		ECS::registry<SpriteSheet>.get(entity).currentAnimationNumber = 0;
	}
	if (env.type == Event::TILE_UNOCCUPIED)
	{
		ECS::registry<SpriteSheet>.get(entity).currentAnimationNumber = 1;
	}
}
