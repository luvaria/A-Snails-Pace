#pragma once

// stlib
#include <vector>
#include "common.hpp"
#include <unordered_map>

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
    struct KeyFuncs
    {
        size_t operator()(const ivec2& k)const
        {
            return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
        }

        bool operator()(const ivec2& a, const ivec2& b)const
        {
            return a.x == b.x && a.y == b.y;
        }
    };


    typedef std::unordered_map<ivec2, Tile, KeyFuncs, KeyFuncs> vec2Map;
    // tile grid
    static std::vector<std::vector<Tile>>& getTiles();
    static vec2Map& getAllTileMovesMap();
};
