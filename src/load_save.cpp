// header
#include "load_save.hpp"
#include "common.hpp"
#include "tiny_ecs.hpp"

// stlib
#include <fstream>
#include <iostream>

// json
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>

char constexpr LoadSaveSystem::COLLECTIBLE_KEY[];
char constexpr LoadSaveSystem::EQUIPPED_KEY[];
char constexpr LoadSaveSystem::POINTS_KEY[];
char constexpr LoadSaveSystem::PLAYER_DIR[];
char constexpr LoadSaveSystem::PLAYER_FILE[];

// for convenience
using json = nlohmann::json;

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

    o << save << std::endl;
}
