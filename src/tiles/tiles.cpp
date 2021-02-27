#include "tiles/tiles.hpp"

// Zoom level; dimensions of tiles. Loaded from each level file
static float scale = 0.f;
static std::vector<std::vector<TileSystem::Tile>> tiles;

float TileSystem::getScale() { return scale; }
void TileSystem::setScale(float s) { scale = s; }
void TileSystem::resetGrid() { tiles.clear(); };
std::vector<std::vector<TileSystem::Tile>>& TileSystem::getTiles() { return tiles; }
