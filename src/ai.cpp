// internal
#include "ai.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"
#include "snail.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "bird.hpp"
#include "common.hpp"
#include "world.hpp"
#include "render_components.hpp"
#include "debug.hpp"
#include "projectile.hpp"

// stlib
#include <iostream>
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
    if (ECS::registry<Turn>.components[0].type == ENEMY) {
        for (unsigned int i = 0; i < aiRegistry.components.size(); i++)
        {
            auto entity = aiRegistry.entities[i];
            auto& tree = aiRegistry.components[i].tree;
            tree->process(entity);

            if (ECS::registry<SuperSpider>.has(entity) == true) {
                auto& fire = ECS::registry<Fire>.get(entity);
                if (fire.fired == true) {
                    fire.fired = false;
                    projectileShoot(entity);
                }
            }
            aiMovedThisStep = true;
        }
        for (auto& entity : ECS::registry<Bird>.entities) {
            auto& fire = ECS::registry<Fire>.get(entity);
            if (fire.fired == true) {
                fire.fired = false;
                projectileShoot(entity);
            }
        }
        if (aiMovedThisStep)
        {
            aiMoved = true;
        }
    }

    if (ECS::registry<Turn>.components[0].type == PLAYER_WAITING) {
        for (int i = 0; i < ECS::registry<Fire>.components.size(); i++) {
            auto& fired = ECS::registry<Fire>.components[i].fired;
            fired = true;
        }
    }

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units; // placeholder to silence unused warning until implemented
}

std::vector<vec2> AISystem::shortestPathBFS(vec2 start, vec2 goal, std::string animal) {
    auto tileMovesMap = TileSystem::getAllTileMovesMap();
    std::vector<vec2> startFrontier;
    startFrontier.push_back(start);
    tileMovesMap.erase(start);
    std::deque<std::vector<vec2>> frontier = {startFrontier};
    std::vector<vec2> current = frontier.front();

  while (!frontier.empty()) {
    current = frontier.front();
    frontier.pop_front();
        if (checkIfReachedDestinationOrAddNeighboringNodesToFrontier(frontier, current, tileMovesMap, goal)) {
            return current;
        }
    
  }
    return startFrontier;
}

std::vector<vec2> AISystem::shortestPathAStar(vec2 start, vec2 goal, std::string animal) {
    auto tileMovesMap = TileSystem::getAllTileMovesMap();
    std::vector<vec2> startFrontier;
    startFrontier.push_back(start);
    tileMovesMap.erase(start);
    std::deque<std::vector<vec2>> frontier = {startFrontier};
    std::vector<vec2> current = frontier.front();

  while (!frontier.empty()) {
    current = frontier.front();
    frontier.pop_front();
        //std::cout << "call to SPIDER" << std::endl;
        if (checkIfReachedDestinationOrAddNeighboringNodesToFrontier(frontier, current, tileMovesMap, goal)) {
            //std::cout << "about to return path" << std::endl;
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

void AISystem::projectileShoot(ECS::Entity& e) {

    // range of bird firing, don't want him to fire if he is off the screen.
    if (ECS::registry<Bird>.has(e)) {
        auto& snailEntity = ECS::registry<Snail>.entities[0];
        float scale = TileSystem::getScale();

        vec2 snailPos = ECS::registry<Motion>.get(snailEntity).position;
        int xPos = (snailPos[0] - (0.5 * scale)) / scale;
        int yPos = (snailPos[1] - (0.5 * scale)) / scale;
        vec2 snailCoord = { yPos, xPos };

        auto tiles = TileSystem::getTiles();
        auto& motion = ECS::registry<Motion>.get(e);
        vec2 aiPos = motion.position;
        int xAiPos = (aiPos.x - (0.5 * scale)) / scale;
        int yAiPos = (aiPos.y - (0.5 * scale)) / scale;
        vec2 aiCoord = { yAiPos, xAiPos };

        if (xAiPos - 5 > xPos) {
            return;
        }
        if (xAiPos + 4 < xPos) {
            return;
        }
        
    }


    vec2 birdPosition = ECS::registry<Motion>.get(e).position;

    auto& snailEntity = ECS::registry<Snail>.entities[0];

    // now you want to go in the direction of the (mouse_pos - snail_pos), but make it a unit vector
    vec2 snailPosition = ECS::registry<Motion>.get(snailEntity).position;
    if (birdPosition == snailPosition) {
        return;
    }
    vec2 normal = normalize(snailPosition - birdPosition);
    float factor = 0.50f * TileSystem::getScale();
    vec2 offset = factor * normal;
    vec2 projectilePosition = birdPosition + offset;
    //vec2 projectilePosition = snailPosition + 0.70f * normalize(mousePos - snailPosition) * TileSystem::getScale();
    vec2 projectileVelocity = (snailPosition - projectilePosition);
    float length = glm::length(projectileVelocity);
    projectileVelocity.x = (projectileVelocity.x / length) * TileSystem::getScale();
    projectileVelocity.y = (projectileVelocity.y / length) * TileSystem::getScale();
    if (projectileVelocity != vec2(0, 0))
    {
        // implement a version of this for the slug
        SlugProjectile::createProjectile(projectilePosition, projectileVelocity);
    }
}

BTState LookForSnail::process(ECS::Entity e) {
    //std::cout << "in look for snail" << std::endl;
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
        current = AISystem::shortestPathAStar(aiCoord, snailCoord, "spider");
    }
    else {
        current = AISystem::shortestPathBFS(aiCoord, snailCoord, "spider");
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
        /*
        if (ECS::registry<SuperSpider>.has(e) == true) {
            std::cout << "super spider" << std::endl;
            AISystem::superSpiderMove(entity, current);
        }
        */
        //else {
            if (aiMove == 0) {
                if (currPos.x - yAiPos > 0) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_SOUTH;
                    WorldSystem::goDown(entity, aiMove);
                }
                else if (currPos.x - yAiPos < 0) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_NORTH;
                    WorldSystem::goUp(entity, aiMove);
                }
                else if (currPos.y - xAiPos > 0) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_EAST;
                    WorldSystem::goRight(entity, aiMove);
                }
                else {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_WEST;
                    WorldSystem::goLeft(entity, aiMove);
                }
            }
            if (aiMove == 0) {
                if (motion.lastDirection == DIRECTION_NORTH) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_NORTH;
                    WorldSystem::goUp(entity, aiMove);
                }
                else if (motion.lastDirection == DIRECTION_SOUTH) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_SOUTH;
                    WorldSystem::goDown(entity, aiMove);
                }
                else if (motion.lastDirection == DIRECTION_EAST) {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_EAST;
                    WorldSystem::goRight(entity, aiMove);
                }
                else {
                    ECS::registry<DirectionInput>.get(entity).direction = DIRECTION_WEST;
                    WorldSystem::goLeft(entity, aiMove);
                }
            }
        //}
        //std::cout << "returning Failure" << std::endl;
        return BTState::Failure;
    }
    //std::cout << "returning Running" << std::endl;
    if (ECS::registry<Slug>.has(e) == true && m_inRange == true) {
        int xAiPos = (aiPos.x - (0.5 * scale)) / scale;
        int yAiPos = (aiPos.y - (0.5 * scale)) / scale;
        vec2 snailLoc = ECS::registry<Motion>.get(snailEntity).position;
        int xLoc = (snailLoc[0] - (0.5 * scale)) / scale;
        int yLoc = (snailLoc[1] - (0.5 * scale)) / scale;
        if (xAiPos - 1 == xLoc && yAiPos == yLoc) {
            return BTState::Success;
        }
        else if (xAiPos + 1 == xLoc && yAiPos == yLoc) {
            return BTState::Success;
        }
    }

    return BTState::Running;
}

void LookForSnail::writeToJson(json &toSave)
{
    toSave[BTKeys::TYPE_KEY] = getNodeType();
    toSave[BTKeys::IN_RANGE_KEY] = m_inRange;
}

void LookForSnail::setFromJson(const json &saved)
{
    m_inRange = saved[BTKeys::IN_RANGE_KEY];
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
    int yMin = aiCoord.y - range;


    if (xMax >= snailCoord.x && xMin <= snailCoord.x) {
        //std::cout << "success1" << std::endl;
        if (yMax >= snailCoord.y && yMin <= snailCoord.y) {
            //std::cout << "success2" << std::endl;
            return BTState::Success;
        }
        else {
            return BTState::Failure;
        }
    }
    else {
        //std::cout << "failure" << std::endl;
        return BTState::Failure;
    }
}

void IsSnailInRange::writeToJson(json &toSave)
{
    toSave[BTKeys::TYPE_KEY] = getNodeType();
}


BTState FireXShots::process(ECS::Entity e) {
    
    if (m_Skip > 0) {
        m_Skip = m_Skip - 1;
        return BTState::Failure;
    }
    //std::cout << "in FireXShots" << std::endl;
    //first we get the position of the mouse_pos relative to the start of the level.
    // get snail position
    vec2 slugPosition = ECS::registry<Motion>.get(e).position;

    auto& snailEntity = ECS::registry<Snail>.entities[0];

    // now you want to go in the direction of the (mouse_pos - snail_pos), but make it a unit vector
    vec2 snailPosition = ECS::registry<Motion>.get(snailEntity).position;

    vec2 projectilePosition = slugPosition + glm::normalize(snailPosition - slugPosition) * TileSystem::getScale() / 2.f;
    vec2 projectileVelocity = (snailPosition - projectilePosition);
    float length = glm::length(projectileVelocity);
    projectileVelocity.x = (projectileVelocity.x / length) * TileSystem::getScale() * 2;
    projectileVelocity.y = (projectileVelocity.y / length) * TileSystem::getScale() * 2;
    if (projectileVelocity != vec2(0, 0))
    {
        // implement a version of this for the slug
        SlugProjectile::createProjectile(projectilePosition, projectileVelocity);
    }

   
    return BTState::Success;
}

void FireXShots::writeToJson(json &toSave)
{
    toSave[BTKeys::TYPE_KEY] = getNodeType();
    toSave[BTKeys::SKIP_KEY] = m_Skip;
}

void FireXShots::setFromJson(const json &saved)
{
    m_Skip = saved[BTKeys::SKIP_KEY];
}

BTState PredictShot::process(ECS::Entity e) {
    //std::cout << "in predict shot" << std::endl;
    return BTState::Success;
}

void PredictShot::writeToJson(json &toSave)
{
    toSave[BTKeys::TYPE_KEY] = getNodeType();
}

BTState GetToSnail::process(ECS::Entity e) {
    return BTState::Success;
}

void GetToSnail::writeToJson(json &toSave)
{
    toSave[BTKeys::TYPE_KEY] = getNodeType();
}

std::shared_ptr<BTNode> BTNode::createSubclassNode(NodeType const& type)
{
    // T.T really hurts the eyes... sorry

    std::shared_ptr<BTNode> node;
    if (type == BTKeys::NodeTypes::SEQUENCE_KEY)
    {
        node = std::make_shared<BTSequence>(std::vector<std::shared_ptr<BTNode>>());
    }
    else if (type == BTKeys::NodeTypes::SELECTOR_KEY)
    {
        node = std::make_shared<BTSelector>(std::vector<std::shared_ptr<BTNode>>());
    }
    else if (type == BTKeys::NodeTypes::REPEAT_FOR_N_KEY)
    {
        node = std::make_shared<RepeatForN>(nullptr, 0);
    }
    else if (type == BTKeys::NodeTypes::SNAIL_IN_RANGE_KEY)
    {
        node = std::make_shared<IsSnailInRange>();
    }
    else if (type == BTKeys::NodeTypes::LOOK_FOR_SNAIL_KEY)
    {
        node = std::make_shared<LookForSnail>();
    }
    else if (type == BTKeys::NodeTypes::FIRE_X_SHOTS_KEY)
    {
        node = std::make_shared<FireXShots>();
    }
    else if (type == BTKeys::NodeTypes::RANDOM_SELECTOR)
    {
        node = std::make_shared<RandomSelector>(0, nullptr, nullptr);
    }
    else if (type == BTKeys::NodeTypes::PREDICT_SHOT_KEY)
    {
        node = std::make_shared<PredictShot>();
    }
    else if (type == BTKeys::NodeTypes::GET_TO_SNAIL_KEY)
    {
        node = std::make_shared<GetToSnail>();
    }
    else
    {
        throw std::runtime_error("Cannot create BTNode subclass because type does not exist. Type: " + type);
    }
    return node;
}

bool AISystem::aiMoved = false;
bool AISystem::fire = true;
std::string AISystem::aiPathFindingAlgorithm = "BFS";
//std::vector<vec2> birdPath = AISystem::getBirdPath();
