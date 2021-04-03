# pragma once

// stlib
#include <string>

// json
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>
#include "common.hpp"

// for convenience
using json = nlohmann::json;

namespace WorldKeys
{
    static std::string const LEVEL_NUM_KEY = "level_num";
    static std::string const TURNS_KEY = "turns_taken";
    static std::string const NUM_DEATHS_KEY = "num_deaths";
    static std::string const NUM_ENEMIES_KILLS_KEY = "num_enemy_kills";
    static std::string const NUM_PROJECTILES_FIRED_KEY = "num_projectiles_fired";
}

class LoadSaveSystem
{
public:
    /* player save constants */
    static char constexpr PLAYER_DIR[] = "player/";
    static char constexpr PLAYER_FILE[] = "player_data.json";

    static char constexpr COLLECTIBLE_KEY[] = "collectibles";
    static char constexpr EQUIPPED_KEY[] = "equipped";
    static char constexpr POINTS_KEY[] = "points";

    /* level save constants */
    static char constexpr LEVEL_DIR[] = "level/";
    static char constexpr LEVEL_FILE[] = "level_data.json";

    static char constexpr CHARACTER_KEY[] = "characters";
    static char constexpr PLAYER_KEY[] = "snail";
    static char constexpr SPIDER_KEY[] = "spider";
    static char constexpr SLUG_KEY[] = "slug";
    static char constexpr CHARACTER_X_POS_KEY[] = "x";
    static char constexpr CHARACTER_Y_POS_KEY[] = "y";


    static void loadPlayerFile();
    static void writePlayerFile();

    static bool levelFileExists();
    static int getSavedLevelNum(); // returns -1 if file doesn't exist
    static json loadLevelFileToJson();
    static void writeLevelFile(json& toSave); // passing partially filled json with world data
    static json makeBaseCharacterJson(Motion const& motion);
};
