// Header
#include "water.hpp"
#include "render.hpp"

ECS::Entity WaterTile::createWaterTile(Tile& tile, ECS::Entity entity)
{
	ECS::registry<Tile>.insert(entity, std::move(tile));
	return createWaterTile(vec2(tile.x, tile.y), entity);
}

ECS::Entity WaterTile::createWaterTile(vec2 position, ECS::Entity entity)
{
    std::string key = "water";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        float numFrames = 2.0f;
        float numAnimations = 1.0f;
        resource.texture.frameSize = { 1.0f / numFrames, 1.0f / numAnimations }; // FRAME SIZE HERE!!! this is the percentage of the whole thing...
        RenderSystem::createSprite(resource, textures_path("water.png"), "spriteSheet", true);
    }

    std::string key_min = "minWater";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("water-min.obj"));
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
    motion.scale = { TileSystem::getScale(), TileSystem::getScale() };

    //setting the animation information
    SpriteSheet& spriteSheet = ECS::registry<SpriteSheet>.emplace(entity);
    spriteSheet.animationSpeed = 160;
    spriteSheet.numAnimationFrames = 2;
    spriteSheet.currentAnimationNumber = 0; //leaves move when the level starts.


    // Create an (empty) VineTile component
    auto& waterTile = ECS::registry<WaterTile>.emplace(entity);
    waterTile.entity = entity;
    return entity;
}

// Credits (WaterSplash): https://opengameart.org/content/water-splash
ECS::Entity WaterTile::createWaterSplashTile(Tile& tile, ECS::Entity entity)
{
    ECS::registry<Tile>.insert(entity, std::move(tile));
    return createWaterSplashTile(vec2(tile.x, tile.y), entity);
}

ECS::Entity WaterTile::createWaterSplashTile(vec2 position, ECS::Entity entity)
{
    std::string key = "waterSplash";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        float numFrames = 7.0f;
        float numAnimations = 1.0f;
        resource.texture.frameSize = { 1.0f / numFrames, 1.0f / numAnimations }; // FRAME SIZE HERE!!! this is the percentage of the whole thing...
        RenderSystem::createSprite(resource, textures_path("waterSplash.png"), "spriteSheet", true);
    }
    
    
    std::string key_min = "minWaterSplash";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("waterSplash-min.obj"));
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::TILE);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::TILE);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = { TileSystem::getScale(), TileSystem::getScale()};

    //setting the animation information
    SpriteSheet& spriteSheet = ECS::registry<SpriteSheet>.emplace(entity);
    spriteSheet.animationSpeed = 200;
    spriteSheet.numAnimationFrames = 7;
    spriteSheet.currentAnimationNumber = 0;
    auto& waterTile = ECS::registry<WaterTile>.emplace(entity);
    waterTile.entity = entity;
    return entity;
}

void WaterTile::onNotify(Event env, ECS::Entity& e)
{
    if (env.type == Event::SPLASH && WaterTile::splashEntityID == 0)
    {
        auto& tiles = TileSystem::getTiles();
        float scale = TileSystem::getScale();

        Motion& motion = ECS::registry<Motion>.get(e);
        vec2 ePos = motion.position;
        int xPos = (ePos[0] - (0.5 * scale)) / scale;
        int yPos = ((ePos[1] - (0.5 * scale)) / scale) - 1;
        if(tiles[yPos+1][xPos].type != SPLASH) {
            auto entity = ECS::Entity();
            Tile& tile = tiles[yPos][xPos];
            tile.x = (float)(xPos * scale + 0.5 * scale);
            tile.y = (float)(yPos * scale + 0.5 * scale);
            tile.type = SPLASH;
            WaterTile::createWaterSplashTile(tile, entity);
            WaterTile::splashEntityID = entity.id;
            tiles[yPos][xPos] = tile;
        }
    }
}
int WaterTile::splashEntityID = 0;
