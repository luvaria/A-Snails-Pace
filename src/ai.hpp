#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class AISystem
{
public:
    static int aiMoves;
	void step(float elapsed_ms, vec2 window_size_in_game_units);
    std::vector<vec2> shortestPathBFS(vec2 start, vec2 goal);

};

