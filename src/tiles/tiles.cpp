#include "tiles/tiles.hpp"

float TileSystem::scale = 0.f;
std::vector<std::vector<Tile>> TileSystem::tiles;
ScrollDirection TileSystem::scrollDirection = LEFT_TO_RIGHT;
static unsigned turns_for_camera_update = 1;

// Possible tile that entity can travel
static TileSystem::vec2Map tileMovesMap;

float TileSystem::getScale() { return scale; }
void TileSystem::setScale(float s) { scale = s; }
void TileSystem::resetGrid() { tiles.clear(); }
unsigned TileSystem::getTurnsForCameraUpdate() { return turns_for_camera_update; }
void TileSystem::setTurnsForCameraUpdate(unsigned turns) { turns_for_camera_update = turns; }
std::vector<std::vector<Tile>>& TileSystem::getTiles() { return tiles; }
ScrollDirection TileSystem::getScrollDirection() { return scrollDirection; }
void TileSystem::setScrollDirection(ScrollDirection dir) { scrollDirection = dir; }
TileSystem::vec2Map& TileSystem::getAllTileMovesMap() { return tileMovesMap; }
