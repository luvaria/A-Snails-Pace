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
    static std::string const CAMERA_KEY = "camera";
}

class LoadSaveSystem
{
public:
    /* player save constants */
    static char constexpr PLAYER_DIR[] = "player/";
    static char constexpr PLAYER_FILE[] = "player_data.json";

    static char constexpr EQUIPPED_KEY[] = "equipped";
    static char constexpr POINTS_KEY[] = "points";

    /* level save constants */
    static char constexpr LEVEL_DIR[] = "level/";
    static char constexpr LEVEL_FILE[] = "level_data.json";

    static char constexpr CHARACTER_KEY[] = "characters";
    static char constexpr PLAYER_KEY[] = "snail";
    static char constexpr SPIDER_KEY[] = "spider";
    static char constexpr SLUG_KEY[] = "slug";

    static char constexpr PROJECTILE_KEY[] = "projectiles";
    static char constexpr PROJECTILE_TYPE_KEY[] = "type";

    static char constexpr CHARACTER_X_POS_KEY[] = "x";
    static char constexpr CHARACTER_Y_POS_KEY[] = "y";
    static char constexpr CHARACTER_ANGLE_KEY[] = "angle";
    static char constexpr CHARACTER_VELOCITY_KEY[] = "velocity";
    static char constexpr CHARACTER_SCALE_KEY[] = "scale";
    static char constexpr CHARACTER_LAST_DIR_KEY[] = "last_dir";

    static char constexpr NPC_KEY[] = "npc";
    static char constexpr NPC_CUR_NODE_KEY[] = "cur_node";
    static char constexpr NPC_CUR_LINE_KEY[] = "cur_line";
    static char constexpr NPC_TIMES_TALKED_KEY[] = "times_talked";

    /* shared constants */
    static char constexpr COLLECTIBLE_KEY[] = "collectibles";


    static void loadPlayerFile();
    static void writePlayerFile();

    static bool levelFileExists();
    static int getSavedLevelNum(); // returns -1 if file doesn't exist
    static json loadLevelFileToJson();
    static void writeLevelFile(json& toSave); // passing partially filled json with world data
    static Motion makeMotionFromJson(json const& motionJson);

private:
    static json makeBaseCharacterJson(Motion const& motion);
};
