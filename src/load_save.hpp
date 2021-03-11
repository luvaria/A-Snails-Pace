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
    static char constexpr collectibleStr[] = "collectibles";

    static void loadPlayerFile(std::string filename);
    static void writePlayerFile(std::string filename);
};
