// header
#include "load_save.hpp"
#include "common.hpp"
#include "tiny_ecs.hpp"

// stlib
#include <fstream>
#include <iostream>

// json
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>

char constexpr LoadSaveSystem::collectibleStr[];

// for convenience
using json = nlohmann::json;

void LoadSaveSystem::loadPlayerFile(std::string filename)
{
    std::ifstream i(save_path(filename));

    if (!i) return;

    json save = json::parse(i);

    for (int id : save[collectibleStr])
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

void LoadSaveSystem::writePlayerFile(std::string filename)
{
    std::ofstream o(save_path(filename));
    json save;

    auto& collectibleReg = ECS::registry<Collectible>;
    for (auto& collectible : collectibleReg.components)
    {
        save[collectibleStr].push_back(collectible.id);
    }

    o << save << std::endl;
}
