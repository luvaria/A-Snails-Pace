#pragma once

#include <vector>
#include <deque>          // std::deque
#include <list>           // std::list
#include <queue>
#include <memory>
#include <type_traits>

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

// The return type of behaviour tree processing
enum class BTState {
    Running,
    Success,
    Failure
};

class BTNode {
    public:
        virtual void init(ECS::Entity e) {};
        virtual BTState process(ECS::Entity e) = 0;
};

// loops through all children and exits when one fails.

class BTSequence : public BTNode {
public:
    BTSequence(std::vector<std::shared_ptr<BTNode>> children)
        : m_children(std::move(children)), m_index(0) {

    }

private:
    void init(ECS::Entity e) override
    {
        m_index = 0;
        assert(m_index < m_children.size());
        // initialize the first child
        const auto& child = m_children[m_index];
        assert(child);
        child->init(e);
    }

    BTState process(ECS::Entity e) override {
        if (m_index >= m_children.size())
            return BTState::Success;

        // process current child
        const auto& child = m_children[m_index];
        assert(child);
        BTState state = child->process(e);

        // select a new active child and initialize its internal state
        if (state == BTState::Success) {
            ++m_index;
            if (m_index >= m_children.size()) {
                return BTState::Success;
            }
            else {
                const auto& nextChild = m_children[m_index];
                assert(nextChild);
                nextChild->init(e);
                return BTState::Running;
            }
        }
        else {
            return state;
        }
    }

    int m_index;
    std::vector<std::shared_ptr<BTNode>> m_children;
};

class IsSnailInRange : public BTNode {

};

class LookForSnail : public BTNode {
public: 
    LookForSnail() noexcept {

    }
private:
    void init(ECS::Entity e) override {

    }
    BTState process(ECS::Entity e) override;
};

class NoPathsPossible : public BTNode {
public:
    NoPathsPossible() noexcept {

    }
private:
    void init(ECS::Entity e) override;

    BTState process(ECS::Entity e) override;
};


class AISystem
{
public:

    static std::string aiPathFindingAlgorithm;
    static bool aiMoved;

	  void step(float elapsed_ms, vec2 window_size_in_game_units);
    void init();
    static std::vector<vec2> shortestPathBFS(vec2 start, vec2 goal);
    static std::vector<vec2> shortestPathAStar(vec2 start, vec2 goal);
    static void sortQueue(std::deque<std::vector<vec2>> &frontier, vec2 destCoord);
    static bool checkIfReachedDestinationOrAddNeighboringNodesToFrontier(std::deque<std::vector<vec2>>& frontier, std::vector<vec2>& current, TileSystem::vec2Map& tileMovesMap, vec2& goal);
};
