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
    static std::string const MSG_INDEX_KEY = "msg_index";
    static std::string const FIRST_RUN_VEC_KEY = "first_run_vec";
}


typedef std::string NodeType;
namespace BTKeys
{
    namespace NodeTypes
    {
        static NodeType const SEQUENCE_KEY = "sequence";
        static NodeType const SELECTOR_KEY = "selector";
        static NodeType const REPEAT_FOR_N_KEY = "repeat_for_n";
        static NodeType const SNAIL_IN_RANGE_KEY = "is_snail_in_range";
        static NodeType const LOOK_FOR_SNAIL_KEY = "look_for_snail";
        static NodeType const FIRE_X_SHOTS_KEY = "fire_x_shots";
        static NodeType const RANDOM_SELECTOR = "random_selector";
        static NodeType const PREDICT_SHOT_KEY = "predict_shot";
        static NodeType const GET_TO_SNAIL_KEY = "get_to_snail";
    }

    static std::string const TYPE_KEY = "type";
    static std::string const INDEX_KEY = "index";
    static std::string const CHILDREN_KEY = "children";
    static std::string const CHILD_KEY = "child";
    static std::string const ITERATIONS_REMAINING_KEY = "iterations_remaining";
    static std::string const IN_RANGE_KEY = "in_range";
    static std::string const SKIP_KEY = "skip";

    // random selector node keys
    static std::string const CHILD_1_KEY = "child_1";
    static std::string const CHILD_2_KEY = "child_2";
    static std::string const CHOSEN_KEY = "chosen";
    static std::string const CHANCE_KEY = "chance";
}

class LoadSaveSystem
{
public:
    /* player save constants */
    static char constexpr PLAYER_DIR[] = "player/";
    static char constexpr PLAYER_FILE[] = "player_data.json";

    static char constexpr EQUIPPED_KEY[] = "equipped";
    static char constexpr POINTS_KEY[] = "points";
    static char constexpr VOLUME_KEY[] = "volume";

    /* level save constants */
    static char constexpr LEVEL_DIR[] = "level/";
    static char constexpr LEVEL_FILE[] = "level_data.json";

    static char constexpr CHARACTER_KEY[] = "characters";
    static char constexpr PLAYER_KEY[] = "snail";
    static char constexpr SPIDER_KEY[] = "spider";
    static char constexpr SLUG_KEY[] = "slug";
    static char constexpr BIRD_KEY[] = "bird";
    static char constexpr FISH_KEY[] = "fish";
    static char constexpr SUPER_SPIDER_KEY[] = "super_spider";

    static char constexpr FISH_MOVE_KEY[] = "move";
    static char constexpr FISH_MOVE_DIRECTION_KEY[] = "direction";
    static char constexpr FISH_MOVE_MOVED_KEY[] = "has_moved";

    static char constexpr BTREE_KEY[] = "btree";

    static char constexpr PROJECTILE_KEY[] = "projectiles";
    static char constexpr PROJECTILE_TYPE_KEY[] = "type";
    static char constexpr PROJECTILE_MOVED_KEY[] = "moved";
    static char constexpr PROJECTILE_ORIG_SCALE[] = "orig_scale";

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


    static double getSavedVolume();
    static void loadPlayerFile();
    static void writePlayerFile();

    static bool levelFileExists();
    static void deleteSaveFile();
    static int getSavedLevelNum(); // returns -1 if file doesn't exist
    static json loadLevelFileToJson();
    static void writeLevelFile(json& toSave); // passing partially filled json with world data
    static Motion makeMotionFromJson(json const& motionJson, bool centreOnTile = true);

private:
    static json makeMotionJson(Motion const& motion, bool savePosAsTile = true);
};
