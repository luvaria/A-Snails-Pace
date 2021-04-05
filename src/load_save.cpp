// header
#include "load_save.hpp"
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "tiles/tiles.hpp"
#include "projectile.hpp"
#include "collectible.hpp"

// stlib
#include <fstream>
#include <iostream>

char constexpr LoadSaveSystem::PLAYER_DIR[];
char constexpr LoadSaveSystem::PLAYER_FILE[];

char constexpr LoadSaveSystem::COLLECTIBLE_KEY[];
char constexpr LoadSaveSystem::EQUIPPED_KEY[];
char constexpr LoadSaveSystem::POINTS_KEY[];

char constexpr LoadSaveSystem::LEVEL_DIR[];
char constexpr LoadSaveSystem::LEVEL_FILE[];

char constexpr LoadSaveSystem::CHARACTER_KEY[];
char constexpr LoadSaveSystem::PLAYER_KEY[];
char constexpr LoadSaveSystem::SPIDER_KEY[];
char constexpr LoadSaveSystem::SLUG_KEY[];

char constexpr LoadSaveSystem::PROJECTILE_KEY[];
char constexpr LoadSaveSystem::PROJECTILE_TYPE_KEY[];

char constexpr LoadSaveSystem::CHARACTER_X_POS_KEY[];
char constexpr LoadSaveSystem::CHARACTER_Y_POS_KEY[];
char constexpr LoadSaveSystem::CHARACTER_ANGLE_KEY[];
char constexpr LoadSaveSystem::CHARACTER_VELOCITY_KEY[];
char constexpr LoadSaveSystem::CHARACTER_SCALE_KEY[];
char constexpr LoadSaveSystem::CHARACTER_LAST_DIR_KEY[];

void LoadSaveSystem::loadPlayerFile()
{
    // initialize the inventory
    auto& inventory = (ECS::registry<Inventory>.size() == 0) ?
                      ECS::registry<Inventory>.emplace(ECS::Entity()) : ECS::registry<Inventory>.components[0];

    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ifstream i(save_path(filename));

    if (!i) return;

    json save = json::parse(i);

    for (int id : save[COLLECTIBLE_KEY])
    {
        inventory.collectibles.insert(id);
    }

    inventory.equipped = save.value(EQUIPPED_KEY, inventory.equipped);
    inventory.points = save.value(POINTS_KEY, inventory.points);
}

void LoadSaveSystem::writePlayerFile()
{
    if (ECS::registry<Inventory>.size() == 0) return;

    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ofstream o(save_path(filename));
    json save;

    auto& inventory = ECS::registry<Inventory>.components[0];
    std::unordered_set<CollectId>& collectibles = inventory.collectibles;

    for (auto& collectible : collectibles)
    {
        save[COLLECTIBLE_KEY].push_back(collectible);
    }

    save[EQUIPPED_KEY] = inventory.equipped;
    save[POINTS_KEY] = inventory.points;

    o << std::setw(2) << save << std::endl;
}

bool LoadSaveSystem::levelFileExists()
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ifstream i(save_path(filename));

    // uwu
    return !(!i);
}

int LoadSaveSystem::getSavedLevelNum()
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ifstream i(save_path(filename));

    if (!i) return -1;

    json save = json::parse(i);
    return save[WorldKeys::LEVEL_NUM_KEY];
}

json LoadSaveSystem::loadLevelFileToJson()
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ifstream i(save_path(filename));

    assert(!(!i)); // file does not exist

    json save = json::parse(i);
    return save;
}

void LoadSaveSystem::writeLevelFile(json& toSave)
{
    // TODO finish this
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ofstream o(save_path(filename));

    for (ECS::Entity player : ECS::registry<Player>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(player);
        json character = makeBaseCharacterJson(motion);
        toSave[CHARACTER_KEY][PLAYER_KEY].push_back(character);
    }

    for (ECS::Entity spider : ECS::registry<Spider>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(spider);
        json character = makeBaseCharacterJson(motion);
        toSave[CHARACTER_KEY][SPIDER_KEY].push_back(character);
    }

    for (ECS::Entity slug : ECS::registry<Slug>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(slug);
        json character = makeBaseCharacterJson(motion);
        toSave[CHARACTER_KEY][SLUG_KEY].push_back(character);
    }

    for (ECS::Entity projectile : ECS::registry<Projectile>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(projectile);
        json character = makeBaseCharacterJson(motion);

        if (ECS::registry<SnailProjectile>.has(projectile))
        {
            character[PROJECTILE_TYPE_KEY] = "snail";
        }
        else if (ECS::registry<SlugProjectile>.has(projectile))
        {
            character[PROJECTILE_TYPE_KEY] = "slug";
        }

        toSave[PROJECTILE_KEY].push_back(character);
    }

    // save unequipped collectibles
    for (ECS::Entity collectible : ECS::registry<Collectible>.entities)
    {
        if (!ECS::registry<NoCollide>.has(collectible)) // hack way of checking if it's not equipped
        {
            auto& motion = ECS::registry<Motion>.get(collectible);
            json character = makeBaseCharacterJson(motion);
            character["id"] = ECS::registry<Collectible>.get(collectible).id;
            toSave[COLLECTIBLE_KEY].push_back(character);
        }
    }

    o << std::setw(2) << toSave << std::endl;
}

Motion LoadSaveSystem::makeMotionFromJson(json const& motionJson)
{
    Motion motion = Motion();
    Tile tile = TileSystem::getTiles()[motionJson[CHARACTER_Y_POS_KEY]][motionJson[CHARACTER_X_POS_KEY]];
    motion.position.x = tile.x;
    motion.position.y = tile.y;
    motion.angle = motionJson[CHARACTER_ANGLE_KEY];
    motion.velocity = { motionJson[CHARACTER_VELOCITY_KEY]["x"], motionJson[CHARACTER_VELOCITY_KEY]["y"] };
    motion.scale = { motionJson[CHARACTER_SCALE_KEY]["x"], motionJson[CHARACTER_SCALE_KEY]["y"] };
    motion.lastDirection = motionJson[CHARACTER_LAST_DIR_KEY];
    return motion;
}

json LoadSaveSystem::makeBaseCharacterJson(Motion const& motion)
{
    json character;
    character[CHARACTER_X_POS_KEY] = static_cast<int>(motion.position.x / TileSystem::getScale());
    character[CHARACTER_Y_POS_KEY] = static_cast<int>(motion.position.y / TileSystem::getScale());
    character[CHARACTER_ANGLE_KEY] = motion.angle;
    character[CHARACTER_VELOCITY_KEY]["x"] = motion.velocity.x;
    character[CHARACTER_VELOCITY_KEY]["y"] = motion.velocity.y;
    character[CHARACTER_SCALE_KEY]["x"] = motion.scale.x;
    character[CHARACTER_SCALE_KEY]["y"] = motion.scale.y;
    character[CHARACTER_LAST_DIR_KEY] = motion.lastDirection;
    return character;
}

