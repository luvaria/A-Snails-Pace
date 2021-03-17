// internal
#include "ai.hpp"
#include "tiny_ecs.hpp"
#include <iostream>
#include "tiles/tiles.hpp"
#include "snail.hpp"
#include "spider.hpp"
#include "common.hpp"
#include "world.hpp"
#include "debug.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

void AISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    auto& snailEntity = ECS::registry<Snail>.entities[0];
    float scale = TileSystem::getScale();
    
    vec2 snailPos = ECS::registry<Motion>.get(snailEntity).position;
    int xPos = (snailPos[0] - (0.5*scale))/scale;
    int yPos = (snailPos[1] - (0.5*scale))/scale;
    vec2 snailCoord = {yPos, xPos};

    bool aiMovedThisStep = false;
    auto& aiRegistry = ECS::registry<AI>;
    for (unsigned int i=0; i< aiRegistry.components.size(); i++)
    {
        //std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>();
        //std::shared_ptr <BTNode> npp = std::make_unique<NoPathsPossible>();
        //std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ lfs }));

        auto entity = aiRegistry.entities[i];
        auto& tree = aiRegistry.components[i].tree;
        auto state = tree->process(entity);
        //std::cout << state << std::endl;
        
        if (state != BTState::Running) {
            aiMovedThisStep = true;
        }
        
    }
    
    if (aiMovedThisStep) {
        aiMoved = true;
    }
    
    
	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units; // placeholder to silence unused warning until implemented
}

std::vector<vec2> AISystem::shortestPathBFS(vec2 start, vec2 goal) {
    auto tileMovesMap = TileSystem::getAllTileMovesMap();
    std::vector<vec2> startFrontier;
    startFrontier.push_back(start);
    tileMovesMap.erase(start);
    std::deque<std::vector<vec2>> frontier = {startFrontier};
    std::vector<vec2> current = frontier.front();

  while (!frontier.empty()) {
    current = frontier.front();
    frontier.pop_front();
    if(checkIfReachedDestinationOrAddNeighboringNodesToFrontier(frontier, current, tileMovesMap, goal)) {
        return current;
    }
  }
    return startFrontier;
}

std::vector<vec2> AISystem::shortestPathAStar(vec2 start, vec2 goal) {
    auto tileMovesMap = TileSystem::getAllTileMovesMap();
    std::vector<vec2> startFrontier;
    startFrontier.push_back(start);
    tileMovesMap.erase(start);
    std::deque<std::vector<vec2>> frontier = {startFrontier};
    std::vector<vec2> current = frontier.front();

  while (!frontier.empty()) {
    current = frontier.front();
    frontier.pop_front();
    if(checkIfReachedDestinationOrAddNeighboringNodesToFrontier(frontier, current, tileMovesMap, goal)) {
        return current;
    }
    sortQueue(frontier, goal);
  }
  return startFrontier;
}
  
void AISystem::sortQueue(std::deque<std::vector<vec2>> &frontier, vec2 destCoord)
{
    std::vector<std::vector<vec2>> list;
    std::vector<vec2> current;
    while (!frontier.empty()) {
      current = frontier.front();
      frontier.pop_front();
      list.push_back(current);
    }
    
    sort(list.begin(), list.end(), FrontierComparator(destCoord));
    
    for (auto& item : list) {
        frontier.push_back(item);
    }
}

bool AISystem::checkIfReachedDestinationOrAddNeighboringNodesToFrontier(std::deque<std::vector<vec2>>& frontier, std::vector<vec2>& current, TileSystem::vec2Map& tileMovesMap, vec2& goal) {
    auto tiles = TileSystem::getTiles();

    if (current[current.size()-1] == goal) {
        vec2 lastVec;
        int counter = 0;
        for (vec2 child : current) {
            
            if (DebugSystem::in_path_debug_mode && counter != 0)
            {
                vec2 scale = { TileSystem::getScale(), -TileSystem::getScale() };
                auto scale_horizontal_line = scale;
                scale_horizontal_line.y *= 0.03f;
                auto scale_vertical_line = scale;
                scale_vertical_line.x *= 0.03f;
                if (abs(child.x-lastVec.x)>0) {
                    float scaleFac = child.x-lastVec.x > 0 ? TileSystem::getScale()/2 : -TileSystem::getScale()/2;
                    Tile t = tiles[lastVec.x][lastVec.y];
                    DebugSystem::createLine({t.x, t.y + scaleFac}, scale_vertical_line);
                } else {
                    float scaleFac = child.y-lastVec.y > 0 ? TileSystem::getScale()/2 : -TileSystem::getScale()/2;
                    Tile t = tiles[lastVec.x][lastVec.y];
                    DebugSystem::createLine({t.x + scaleFac, t.y}, scale_horizontal_line);
                }
            }
                
            counter++;
            lastVec = child;
            
        }
        //std::cout << "on true" << std::endl;
        return true;
    }
    else {
        vec2 endNode = current[current.size() - 1];
        std::vector<vec2> next = current;

        // wall above
        if(endNode.x-1 >= 0 && endNode.x-1 < tiles.size() && endNode.y >= 0 && endNode.y < tiles[endNode.x-1].size() && tiles[endNode.x-1][endNode.y].type == WALL) {
            
            if (tileMovesMap.find({endNode.x-1, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x][endNode.y-1].type == EMPTY) {
                    next.push_back({endNode.x, endNode.y-1});
                    next.push_back({endNode.x-1, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y-1});
                }
            }
            if (tileMovesMap.find({endNode.x-1, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x][endNode.y+1].type == EMPTY) {
                    next.push_back({endNode.x, endNode.y+1});
                    next.push_back({endNode.x-1, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y+1});
                }
            }
            
            // left tile
            if (tileMovesMap.find({endNode.x, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y-1].type == WALL) {
                    next.push_back({endNode.x, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x, endNode.y-1});
                }
            }
            
            // right tile
            if (tileMovesMap.find({endNode.x, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y+1].type == WALL) {
                    next.push_back({endNode.x, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x, endNode.y+1});
                }
            }
            
        }
        // wall on the right
        if(endNode.x >= 0 && endNode.x < tiles.size() && endNode.y+1 >= 0 && endNode.y+1 < tiles[endNode.x].size() && tiles[endNode.x][endNode.y+1].type == WALL) {
            if (tileMovesMap.find({endNode.x+1, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y].type == EMPTY) {
                    next.push_back({endNode.x+1, endNode.y});
                    next.push_back({endNode.x+1, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y+1});
                }
            }
            if (tileMovesMap.find({endNode.x-1, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y].type == EMPTY) {
                    next.push_back({endNode.x-1, endNode.y});
                    next.push_back({endNode.x-1, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y+1});
                }
            }
            // up tile
            if (tileMovesMap.find({endNode.x-1, endNode.y}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y+1].type == WALL) {
                    next.push_back({endNode.x-1, endNode.y});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y});
                }
            }
            
            // down tile
            if (tileMovesMap.find({endNode.x+1, endNode.y}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y+1].type == WALL) {
                    next.push_back({endNode.x+1, endNode.y});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y});
                }
            }
        }
        // wall on the left
        if(endNode.x >= 0 && endNode.x < tiles.size() && endNode.y-1 >= 0 && endNode.y-1 < tiles[endNode.x].size() && tiles[endNode.x][endNode.y-1].type == WALL) {
            if (tileMovesMap.find({endNode.x-1, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y].type == EMPTY) {
                    next.push_back({endNode.x-1, endNode.y});
                    next.push_back({endNode.x-1, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y-1});
                }
            }
            if (tileMovesMap.find({endNode.x+1, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y].type == EMPTY) {
                    next.push_back({endNode.x+1, endNode.y});
                    next.push_back({endNode.x+1, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y-1});
                }
            }
            
            // up tile
            if (tileMovesMap.find({endNode.x-1, endNode.y}) != tileMovesMap.end()) {
                if(tiles[endNode.x-1][endNode.y-1].type == WALL) {
                    next.push_back({endNode.x-1, endNode.y});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x-1, endNode.y});
                }
            }
            
            // down tile
            if (tileMovesMap.find({endNode.x+1, endNode.y}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y-1].type == WALL) {
                    next.push_back({endNode.x+1, endNode.y});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y});
                }
            }
        }
        // wall below
        if(endNode.x+1 >= 0 && endNode.x+1 < tiles.size() && endNode.y >= 0 && endNode.y < tiles[endNode.x+1].size() && tiles[endNode.x+1][endNode.y].type == WALL) {
            if (tileMovesMap.find({endNode.x+1, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x][endNode.y+1].type == EMPTY) {
                    next.push_back({endNode.x, endNode.y+1});
                    next.push_back({endNode.x+1, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y-1});
                }
            }
            if (tileMovesMap.find({endNode.x+1, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x][endNode.y-1].type == EMPTY) {
                    next.push_back({endNode.x, endNode.y-1});
                    next.push_back({endNode.x+1, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x+1, endNode.y-1});
                }
            }
            // left tile
            if (tileMovesMap.find({endNode.x, endNode.y-1}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y-1].type == WALL) {
                    next.push_back({endNode.x, endNode.y-1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x, endNode.y-1});
                }
            }
            
            // right tile
            if (tileMovesMap.find({endNode.x, endNode.y+1}) != tileMovesMap.end()) {
                if(tiles[endNode.x+1][endNode.y+1].type == WALL) {
                    next.push_back({endNode.x, endNode.y+1});
                    frontier.push_back(next);
                    next = current;
                    tileMovesMap.erase({endNode.x, endNode.y+1});
                }
            }
        }
        
        
        // INDIVIDUAL //
        
        // left vine tile
        if (tileMovesMap.find({endNode.x, endNode.y-1}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y-1].type == VINE) {
                next.push_back({endNode.x, endNode.y-1});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x, endNode.y-1});
            }
        }
        
        // right vine tile
        if (tileMovesMap.find({endNode.x, endNode.y+1}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y+1].type == VINE) {
                next.push_back({endNode.x, endNode.y+1});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x, endNode.y+1});
            }
        }
        
        // up vine tile
        if (tileMovesMap.find({endNode.x-1, endNode.y}) != tileMovesMap.end()) {
            if(tiles[endNode.x-1][endNode.y].type == VINE) {
                next.push_back({endNode.x-1, endNode.y});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x-1, endNode.y});
            }
        }
        
        // down vine tile
        if (tileMovesMap.find({endNode.x+1, endNode.y}) != tileMovesMap.end()) {
            if(tiles[endNode.x+1][endNode.y].type == VINE) {
                next.push_back({endNode.x+1, endNode.y});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x+1, endNode.y});
            }
        }
        
        // CURRENT VINE TILE
        
        // left vine tile
        if (tileMovesMap.find({endNode.x, endNode.y-1}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y].type == VINE && (tiles[endNode.x+1][endNode.y-1].type == WALL || tiles[endNode.x-1][endNode.y-1].type == WALL)) {
                next.push_back({endNode.x, endNode.y-1});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x, endNode.y-1});
            }
        }
        
        // right vine tile
        if (tileMovesMap.find({endNode.x, endNode.y+1}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y].type == VINE && (tiles[endNode.x-1][endNode.y+1].type == WALL || tiles[endNode.x+1][endNode.y+1].type == WALL)) {
                next.push_back({endNode.x, endNode.y+1});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x, endNode.y+1});
            }
        }
        
        // up vine tile
        if (tileMovesMap.find({endNode.x-1, endNode.y}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y].type == VINE && (tiles[endNode.x-1][endNode.y-1].type == WALL || tiles[endNode.x-1][endNode.y+1].type == WALL)) {
                next.push_back({endNode.x-1, endNode.y});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x-1, endNode.y});
            }
        }
        
        // down vine tile
        if (tileMovesMap.find({endNode.x+1, endNode.y}) != tileMovesMap.end()) {
            if(tiles[endNode.x][endNode.y].type == VINE && (tiles[endNode.x+1][endNode.y-1].type == WALL || tiles[endNode.x+1][endNode.y+1].type == WALL)) {
                next.push_back({endNode.x+1, endNode.y});
                frontier.push_back(next);
                next = current;
                tileMovesMap.erase({endNode.x+1, endNode.y});
            }
        }
        
    }
    //std::cout << "on false" << std::endl;
    return false;

}

BTState LookForSnail::process(ECS::Entity e) {
    // before for loop
    auto& snailEntity = ECS::registry<Snail>.entities[0];
    float scale = TileSystem::getScale();

    vec2 snailPos = ECS::registry<Motion>.get(snailEntity).position;
    int xPos = (snailPos[0] - (0.5 * scale)) / scale;
    int yPos = (snailPos[1] - (0.5 * scale)) / scale;
    vec2 snailCoord = { yPos, xPos };

    // after for loop
    auto entity = e;

    auto tiles = TileSystem::getTiles();
    auto& motion = ECS::registry<Motion>.get(entity);
    vec2 aiPos = motion.position;
    int xAiPos = (aiPos.x - (0.5 * scale)) / scale;
    int yAiPos = (aiPos.y - (0.5 * scale)) / scale;
    vec2 aiCoord = { yAiPos, xAiPos };
    std::vector<vec2> current;

    auto start = std::chrono::high_resolution_clock::now();

    if (AISystem::aiPathFindingAlgorithm == AI_PF_ALGO_A_STAR) {
        current = AISystem::shortestPathAStar(aiCoord, snailCoord);
    }
    else {
        current = AISystem::shortestPathBFS(aiCoord, snailCoord);
    }

    // Get ending timepoint
    auto stop = std::chrono::high_resolution_clock::now();

    // Get duration. Substart timepoints to
    // get durarion. To cast it to proper unit
    // use duration cast method
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    if (DebugSystem::in_path_debug_mode) {
        std::cout << "Time taken by function: "
            << duration.count() << " microseconds" << std::endl;
    }

    if (ECS::registry<Turn>.components[0].type == ENEMY && !AISystem::aiMoved) {
        int aiMove = current.size() == 1 ? 1 : 0;
        vec2 currPos = current.size() > 1 ? current[1] : current[0];
        if (aiMove == 0) {
            if (currPos.x - yAiPos > 0) {
                WorldSystem::goDown(entity, aiMove);
            }
            else if (currPos.x - yAiPos < 0) {
                WorldSystem::goUp(entity, aiMove);
            }
            else if (currPos.y - xAiPos > 0) {
                WorldSystem::goRight(entity, aiMove);
            }
            else {
                WorldSystem::goLeft(entity, aiMove);
            }
        }
        if (aiMove == 0) {
            if (motion.lastDirection == DIRECTION_NORTH) {
                WorldSystem::goUp(entity, aiMove);
            }
            else if (motion.lastDirection == DIRECTION_SOUTH) {
                WorldSystem::goDown(entity, aiMove);
            }
            else if (motion.lastDirection == DIRECTION_EAST) {
                WorldSystem::goRight(entity, aiMove);
            }
            else {
                WorldSystem::goLeft(entity, aiMove);
            }
        }
        return BTState::Failure;
    }
    
    return BTState::Running;
}

BTState IsSnailInRange::process(ECS::Entity e) {
    //std::cout << "checking if snail is in range" << std::endl;
    int range = 7;
    // snail coordinates
    auto& snailEntity = ECS::registry<Snail>.entities[0];
    float scale = TileSystem::getScale();

    vec2 snailPos = ECS::registry<Motion>.get(snailEntity).position;
    int xPos = (snailPos[0] - (0.5 * scale)) / scale;
    int yPos = (snailPos[1] - (0.5 * scale)) / scale;
    vec2 snailCoord = { yPos, xPos };

    auto entity = e;

    auto tiles = TileSystem::getTiles();
    auto& motion = ECS::registry<Motion>.get(entity);
    vec2 aiPos = motion.position;
    int xAiPos = (aiPos.x - (0.5 * scale)) / scale;
    int yAiPos = (aiPos.y - (0.5 * scale)) / scale;
    vec2 aiCoord = { yAiPos, xAiPos };

    int xMax = aiCoord.x + range;
    int xMin = aiCoord.x - range;
    int yMax = aiCoord.y + range;
    int yMin = aiCoord.x - range;


    if (xMax >= snailCoord.x && xMin <= snailCoord.x) {
        //std::cout << "success1" << std::endl;
        return BTState::Success;
    }
    else if (yMax <= snailCoord.y && yMin >= snailCoord.y) {
        //std::cout << "success2" << std::endl;
        return BTState::Success;
    }
    else {
        //std::cout << "failure" << std::endl;
        return BTState::Failure;
    }
}

bool AISystem::aiMoved = false;
std::string AISystem::aiPathFindingAlgorithm = "BFS";
