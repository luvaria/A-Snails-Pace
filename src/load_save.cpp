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

    for (int id : save[COLLECTIBLE_KEY])
    {
        ECS::Entity entity;
        ECS::registry<Collectible>.emplace(entity, id);
    }

    // TODO #54: remove this, temp for debugging
    for (auto& com : ECS::registry<Collectible>.components)
    {
        std::cout << com.id << std::endl;
    }

}

void LoadSaveSystem::writePlayerFile()
{
    std::string const filename = std::string(PLAYER_DIR) + std::string(PLAYER_FILE);
    std::ofstream o(save_path(filename));
    json save;
    std::cout << save_path(filename) << std::endl;

    auto& collectibleReg = ECS::registry<Collectible>;

    ECS::Entity e;
    collectibleReg.emplace(e, 70);

    for (auto& collectible : collectibleReg.components)
    {
        save[COLLECTIBLE_KEY].push_back(collectible.id);
    }

    o << save << std::endl;
}
