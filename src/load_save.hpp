# pragma once

// stlib
#include <string>

// TODO #54: flesh out collectibles
struct Collectible
{
    Collectible(int id) : id(id) {};
    int id;
};

class LoadSaveSystem
{
public:
    static char constexpr COLLECTIBLE_KEY[] = "collectibles";

    static char constexpr PLAYER_DIR[] = "player/";

    static char constexpr PLAYER_FILE[] = "player_data.json";

    static void loadPlayerFile();
    static void writePlayerFile();
};
