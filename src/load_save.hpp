# pragma once

// stlib
#include <string>

class LoadSaveSystem
{
public:
    static char constexpr COLLECTIBLE_KEY[] = "collectibles";
    static char constexpr EQUIPPED_KEY[] = "equipped";
    static char constexpr POINTS_KEY[] = "points";
    static char constexpr VOLUME_KEY[] = "volume";

    static char constexpr PLAYER_DIR[] = "player/";

    static char constexpr PLAYER_FILE[] = "player_data.json";

    static double getSavedVolume();
    static void loadPlayerFile();
    static void writePlayerFile();
};
