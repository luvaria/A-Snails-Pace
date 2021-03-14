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
char constexpr LoadSaveSystem::PLAYER_DIR[];
char constexpr LoadSaveSystem::PLAYER_FILE[];

// for convenience
using json = nlohmann::json;

void LoadSaveSystem::loadPlayerFile()
{
    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ifstream i(save_path(filename));

    if (!i) return;

    json save = json::parse(i);

    if (ECS::registry<Inventory>.size() == 0)
    {
        ECS::Entity entity;
        ECS::registry<Inventory>.emplace(entity);
    }
    std::unordered_set<CollectId>& collectibles = ECS::registry<Inventory>.components[0].collectibles;

    for (int id : save[COLLECTIBLE_KEY])
    {
        collectibles.insert(id);
    }

    // TODO #54: remove this, temp for debugging
//    for (auto& com : collectibles)
//    {
//        std::cout << com << std::endl;
//    }

}

void LoadSaveSystem::writePlayerFile()
{
    if (ECS::registry<Inventory>.size() == 0) return;

    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ofstream o(save_path(filename));
    json save;

    std::unordered_set<CollectId>& collectibles = ECS::registry<Inventory>.components[0].collectibles;

//    TODO #54: remove this, temp for debugging
    for (int i = 0; i < 10; i++)
    {
        collectibles.insert(3 * i);
    }

    for (auto& collectible : collectibles)
    {
        save[COLLECTIBLE_KEY].push_back(collectible);
    }

    o << save << std::endl;
}
