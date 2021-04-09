// header
#include "load_save.hpp"
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "tiles/tiles.hpp"
#include "projectile.hpp"
#include "collectible.hpp"
#include "npc.hpp"
#include "render_components.hpp"
#include "ai.hpp"
#include "bird.hpp"
#include "fish.hpp"

// stlib
#include <fstream>
#include <iostream>
#include <iomanip>

char constexpr LoadSaveSystem::PLAYER_DIR[];
char constexpr LoadSaveSystem::PLAYER_FILE[];

char constexpr LoadSaveSystem::COLLECTIBLE_KEY[];
char constexpr LoadSaveSystem::EQUIPPED_KEY[];
char constexpr LoadSaveSystem::POINTS_KEY[];
char constexpr LoadSaveSystem::VOLUME_KEY[];

char constexpr LoadSaveSystem::LEVEL_DIR[];
char constexpr LoadSaveSystem::LEVEL_FILE[];

char constexpr LoadSaveSystem::CHARACTER_KEY[];
char constexpr LoadSaveSystem::PLAYER_KEY[];
char constexpr LoadSaveSystem::SPIDER_KEY[];
char constexpr LoadSaveSystem::SLUG_KEY[];
char constexpr LoadSaveSystem::BIRD_KEY[];
char constexpr LoadSaveSystem::FISH_KEY[];
char constexpr LoadSaveSystem::SUPER_SPIDER_KEY[];

char constexpr LoadSaveSystem::FISH_MOVE_KEY[];
char constexpr LoadSaveSystem::FISH_MOVE_DIRECTION_KEY[];
char constexpr LoadSaveSystem::FISH_MOVE_MOVED_KEY[];

char constexpr LoadSaveSystem::BTREE_KEY[];

char constexpr LoadSaveSystem::PROJECTILE_KEY[];
char constexpr LoadSaveSystem::PROJECTILE_TYPE_KEY[];

char constexpr LoadSaveSystem::CHARACTER_X_POS_KEY[];
char constexpr LoadSaveSystem::CHARACTER_Y_POS_KEY[];
char constexpr LoadSaveSystem::CHARACTER_ANGLE_KEY[];
char constexpr LoadSaveSystem::CHARACTER_VELOCITY_KEY[];
char constexpr LoadSaveSystem::CHARACTER_SCALE_KEY[];
char constexpr LoadSaveSystem::CHARACTER_LAST_DIR_KEY[];

char constexpr LoadSaveSystem::NPC_KEY[];
char constexpr LoadSaveSystem::NPC_CUR_NODE_KEY[];
char constexpr LoadSaveSystem::NPC_CUR_LINE_KEY[];
char constexpr LoadSaveSystem::NPC_TIMES_TALKED_KEY[];

double LoadSaveSystem::getSavedVolume()
{
    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ifstream i(save_path(filename));

    // file doesn't exist return default
    if (!i.good())
        return 1.f;

    json save = json::parse(i);

    return save.value(VOLUME_KEY, 1.f);
}

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

    Volume::set(save.value(VOLUME_KEY, 1.f));
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

    save[VOLUME_KEY] = Volume::getCur();

    o << std::setw(2) << save << std::endl;
}

bool LoadSaveSystem::levelFileExists()
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ifstream i(save_path(filename));

    return i.good();
}

void LoadSaveSystem::deleteSaveFile()
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::string const fullFilename = save_path(filename);
    std::ifstream i(fullFilename);

    if (i.good())
    {
        i.close();
    }
    else
    {
        // file does not exist
        return;
    }

    if (remove(fullFilename.c_str()) != 0)
    {
        std::string msg = "failed to remove save file: " + fullFilename;
        perror(msg.c_str());
    }
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

    assert(i.good()); // file does not exist

    json save = json::parse(i);
    return save;
}

void LoadSaveSystem::writeLevelFile(json& toSave)
{
    std::string const filename = std::string(LEVEL_DIR) + std::string(LEVEL_FILE);
    std::ofstream o(save_path(filename));

    for (ECS::Entity player : ECS::registry<Player>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(player);
        json character = makeMotionJson(motion);
        toSave[CHARACTER_KEY][PLAYER_KEY].push_back(character);
    }

    for (ECS::Entity spider : ECS::registry<Spider>.entities)
    {
        // exclude exploding spiders
        if (!ECS::registry<DeathTimer>.has(spider))
        {
            auto& motion = ECS::registry<Motion>.get(spider);
            json character = makeMotionJson(motion);

            std::shared_ptr<BTNode> tree = ECS::registry<AI>.get(spider).tree;
            json treeJson;
            tree->writeToJson(treeJson);
            character[BTREE_KEY] = treeJson;

            toSave[CHARACTER_KEY][SPIDER_KEY].push_back(character);
        }
    }

    for (ECS::Entity slug : ECS::registry<Slug>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(slug);
        json character = makeMotionJson(motion);

        std::shared_ptr<BTNode> tree = ECS::registry<AI>.get(slug).tree;
        json treeJson;
        tree->writeToJson(treeJson);
        character[BTREE_KEY] = treeJson;

        toSave[CHARACTER_KEY][SLUG_KEY].push_back(character);
    }

    for (ECS::Entity bird : ECS::registry<Bird>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(bird);
        json character = makeMotionJson(motion);

        toSave[CHARACTER_KEY][BIRD_KEY].push_back(character);
    }

    for (ECS::Entity fish : ECS::registry<Fish>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(fish);
        json character = makeMotionJson(motion);

        json move;
        ECS::registry<Fish::Move>.get(fish).writeToJson(move);
        character[FISH_MOVE_KEY] = move;

        toSave[CHARACTER_KEY][FISH_KEY].push_back(character);
    }

    for (ECS::Entity super_spider : ECS::registry<SuperSpider>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(super_spider);
        json character = makeMotionJson(motion);

        std::shared_ptr<BTNode> tree = ECS::registry<AI>.get(super_spider).tree;
        json treeJson;
        tree->writeToJson(treeJson);
        character[BTREE_KEY] = treeJson;

        toSave[CHARACTER_KEY][SUPER_SPIDER_KEY].push_back(character);
    }

    for (ECS::Entity projectile : ECS::registry<Projectile>.entities)
    {
        auto& motion = ECS::registry<Motion>.get(projectile);
        json character = makeMotionJson(motion, false);

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
            json character = makeMotionJson(motion);
            character["id"] = ECS::registry<Collectible>.get(collectible).id;
            toSave[COLLECTIBLE_KEY].push_back(character);
        }
    }

    // npc is much more different because we save the position as a key to make it easier to match up in level loading
    for (ECS::Entity npc : ECS::registry<NPC>.entities)
    {
        NPC component = ECS::registry<NPC>.get(npc);
        auto position = ECS::registry<Motion>.get(npc).position;
        std::string posKey = std::to_string(static_cast<int>(position.x / TileSystem::getScale()))
                + "," + std::to_string(static_cast<int>(position.y / TileSystem::getScale()));

        json npcJson;
        npcJson[NPC_CUR_NODE_KEY] = component.curNode;
        npcJson[NPC_CUR_LINE_KEY] = component.curLine;
        npcJson[NPC_TIMES_TALKED_KEY] = component.timesTalkedTo;
        toSave[NPC_KEY][posKey] = npcJson;
    }

    o << std::setw(2) << toSave << std::endl;
}

Motion LoadSaveSystem::makeMotionFromJson(json const& motionJson, bool centreOnTile /* = true */)
{
    Motion motion = Motion();
    if (centreOnTile)
    {
        Tile tile = TileSystem::getTiles()[motionJson[CHARACTER_Y_POS_KEY]][motionJson[CHARACTER_X_POS_KEY]];
        motion.position.x = tile.x;
        motion.position.y = tile.y;
    }
    else
    {
        motion.position = { motionJson[CHARACTER_X_POS_KEY], motionJson[CHARACTER_Y_POS_KEY] };
    }

    motion.angle = motionJson[CHARACTER_ANGLE_KEY];
    motion.velocity = { motionJson[CHARACTER_VELOCITY_KEY]["x"], motionJson[CHARACTER_VELOCITY_KEY]["y"] };
    motion.scale = { motionJson[CHARACTER_SCALE_KEY]["x"], motionJson[CHARACTER_SCALE_KEY]["y"] };
    motion.lastDirection = motionJson[CHARACTER_LAST_DIR_KEY];
    return motion;
}

json LoadSaveSystem::makeMotionJson(Motion const& motion, bool savePosAsTile /* = true */)
{
    json character;
    if (savePosAsTile)
    {
        character[CHARACTER_X_POS_KEY] = static_cast<int>(motion.position.x / TileSystem::getScale());
        character[CHARACTER_Y_POS_KEY] = static_cast<int>(motion.position.y / TileSystem::getScale());
    }
    else
    {
        character[CHARACTER_X_POS_KEY] = motion.position.x;
        character[CHARACTER_Y_POS_KEY] = motion.position.y;
    }
    character[CHARACTER_ANGLE_KEY] = motion.angle;
    character[CHARACTER_VELOCITY_KEY]["x"] = motion.velocity.x;
    character[CHARACTER_VELOCITY_KEY]["y"] = motion.velocity.y;
    character[CHARACTER_SCALE_KEY]["x"] = motion.scale.x;
    character[CHARACTER_SCALE_KEY]["y"] = motion.scale.y;
    character[CHARACTER_LAST_DIR_KEY] = motion.lastDirection;
    return character;
}

