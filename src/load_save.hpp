# pragma once

// stlib
#include <string>

class LoadSaveSystem
{
public:
    static char constexpr COLLECTIBLE_KEY[] = "collectibles";
    static char constexpr EQUIPPED_KEY[] = "equipped";
    static char constexpr POINTS_KEY[] = "points";

    static char constexpr PLAYER_DIR[] = "player/";
    static char constexpr LEVEL_DIR[] = "level/";

    static char constexpr PLAYER_FILE[] = "player_data.json";
    static char constexpr LEVEL_FILE[] = "level_data.json";

    static void loadPlayerFile();
    static void writePlayerFile();

    static bool levelFileExists();
};
