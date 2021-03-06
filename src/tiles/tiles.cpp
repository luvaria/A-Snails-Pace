#include "tiles/tiles.hpp"

// Zoom level; dimensions of tiles. Loaded from each level file
static float scale = 0.f;
static unsigned turns_for_camera_update = 1;
static std::vector<std::vector<Tile>> tiles;

// Possible tile that entity can travel
static TileSystem::vec2Map tileMovesMap;

float TileSystem::getScale() { return scale; }
void TileSystem::setScale(float s) { scale = s; }
void TileSystem::resetGrid() { tiles.clear(); }
unsigned TileSystem::getTurnsForCameraUpdate() { return turns_for_camera_update; }
void TileSystem::setTurnsForCameraUpdate(unsigned turns) { turns_for_camera_update = turns; }
std::vector<std::vector<Tile>>& TileSystem::getTiles() { return tiles; }
TileSystem::vec2Map& TileSystem::getAllTileMovesMap() { return tileMovesMap; }
