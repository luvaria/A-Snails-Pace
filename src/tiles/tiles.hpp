#pragma once

// stlib
#include <vector>

class TileSystem
{
public:
	// Defining what tile types are possible, used to render correct tile types
	// and to check if can be moved to. EMPTY = nothing is on the tile and you
	// can move to it if it is unoccupied

	typedef enum { EMPTY, WALL, WATER, VINE } TYPE;

	// Defines the Tile Component
	struct Tile {
		float x;
		float y;
		TYPE type = EMPTY;
		bool occupied = false;
	};

	// size of each (square) tile
	static float getScale();
	static void setScale(float s);

	// reset tile grid (for level loading)
	static void resetGrid();

	// tile grid
	static std::vector<std::vector<TileSystem::Tile>>& getTiles();
};
