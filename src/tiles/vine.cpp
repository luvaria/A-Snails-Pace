// Header
#include "vine.hpp"
#include "render.hpp"

ECS::Entity VineTile::createVineTile(Tile& tile, ECS::Entity entity)
{
	ECS::registry<Tile>.insert(entity, std::move(tile));
	return createVineTile(vec2(tile.x, tile.y), entity);
}

ECS::Entity VineTile::createVineTile(vec2 position, ECS::Entity entity)
{
	std::string key = "vine";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		float numFrames = 2.0f;
		float numAnimations = 1.0f;
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
	spriteSheet.animationSpeed = 300;
	spriteSheet.numAnimationFrames = 2;

	// Create an (empty) VineTile component
	ECS::registry<VineTile>.emplace(entity);

	return entity;
}
