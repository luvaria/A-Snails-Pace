#pragma once

// stlib
#include <vector>

// Defining what tile types are possible, used to render correct tile types
// and to check if can be moved to. EMPTY = nothing is on the tile and you
// can move to it if it is unoccupied
typedef enum { EMPTY, WALL, WATER, VINE } TYPE;

// Defines the tile component
struct Tile {
	float x = 0;
	float y = 0;
	TYPE type = EMPTY;
	bool occupied = false;
};

class TileSystem
{
public:
	// size of each (square) tile
	static float getScale();
	static void setScale(float s);

	// reset tile grid (for level loading)
	static void resetGrid();

	// tile grid
	static std::vector<std::vector<Tile>>& getTiles();
};
