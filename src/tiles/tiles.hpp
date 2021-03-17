#pragma once

#include "common.hpp"

// stlib
#include <vector>
#include "common.hpp"
#include "event.hpp"
#include "subject.hpp"
#include <unordered_map>

// Defining what tile types are possible, used to render correct tile types
// and to check if can be moved to. EMPTY = nothing is on the tile and you
// can move to it if it is unoccupied
typedef enum { EMPTY, WALL, WATER, VINE, SPLASH, INACCESSIBLE } TYPE;

// Defines the tile component
class Tile: public Subject {
public:
    float x = 0;
    float y = 0;
    TYPE type = EMPTY;
    int numOccupyingEntities;

    Tile() 
    {
        numOccupyingEntities = 0;
    }

    void addOccupyingEntity() 
    {
        numOccupyingEntities++;
        if (numOccupyingEntities == 1) 
        {
            notify(Event(Event::TILE_OCCUPIED));
        }
    }

    void removeOccupyingEntity()
    {
        if (numOccupyingEntities == 0) 
        {
            throw std::runtime_error("tried to remove an occupying entity when there were none");
        }
        numOccupyingEntities--;
        if (numOccupyingEntities == 0)
        {
            notify(Event(Event::TILE_UNOCCUPIED));
        }
    }

    bool operator==(const Tile& rhs) const
    {
        return
            this->x == rhs.x &&
            this->y == rhs.y;
    }
};

class TileSystem
{
public:
    // size of each (square) tile
    static float getScale();
    static void setScale(float s);

	// number of turns before camera moves
	static unsigned getTurnsForCameraUpdate();
    static void setTurnsForCameraUpdate(unsigned turns);

    // level end
    static ivec2 getEndCoordinates();
    static void setEndCoordinates(ivec2 coordinates);

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

	// level scrolling direction
	static ScrollDirection getScrollDirection();
	static void setScrollDirection(ScrollDirection dir);

	static vec2Map& getAllTileMovesMap();

private:
	static float scale;
	static std::vector<std::vector<Tile>> tiles;
	static ScrollDirection scrollDirection;
};
