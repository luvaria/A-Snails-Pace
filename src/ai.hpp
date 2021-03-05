#pragma once

#include <vector>
#include <deque>          // std::deque
#include <list>           // std::list
#include <queue>
#include "common.hpp"
#include "tiles/tiles.hpp"
#include "tiny_ecs.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
struct AI {
};

struct FrontierComparator {
    explicit FrontierComparator(vec2 destCoord_)
    : destCoord(destCoord_) {}

    bool operator ()(std::vector<vec2> const& a, std::vector<vec2> const& b) const {
        float distanceA = a.size() + abs(a[a.size() - 1].x - destCoord.x) + abs(a[a.size() - 1].y - destCoord.y);
        float distanceB = b.size() + abs(b[b.size() - 1].x - destCoord.x) + abs(b[b.size() - 1].y - destCoord.y);

        return distanceA < distanceB;
    }

    vec2 destCoord;
};

class AISystem
{
public:

    static std::string aiPathFindingAlgorithm;
    static bool aiMoved;

	void step(float elapsed_ms, vec2 window_size_in_game_units);
    
    std::vector<vec2> shortestPathBFS(vec2 start, vec2 goal);
    std::vector<vec2> shortestPathAStar(vec2 start, vec2 goal);
    void sortQueue(std::deque<std::vector<vec2>> &frontier, vec2 destCoord);
    bool checkIfReachedDestinationOrAddNeighboringNodesToFrontier(std::deque<std::vector<vec2>>& frontier, std::vector<vec2>& current, TileSystem::vec2Map& tileMovesMap, vec2& goal);
};

