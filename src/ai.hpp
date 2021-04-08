#pragma once

#include <vector>
#include <deque>          // std::deque
#include <list>           // std::list
#include <queue>
#include <memory>
#include <type_traits>
#include <iostream>
#include <functional>

#include "common.hpp"
#include "tiles/tiles.hpp"
#include "tiny_ecs.hpp"
#include "load_save.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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
        virtual ~BTNode() {};
        virtual void init(ECS::Entity e) {};
        virtual BTState process(ECS::Entity e) = 0;
        virtual void writeToJson(json& toSave) = 0;
        virtual void setFromJson(json const& saved) = 0;
        virtual NodeType getNodeType() = 0;

        static std::shared_ptr<BTNode> createSubclassNode(NodeType const& type);
};

struct AI {
    std::shared_ptr <BTNode> tree;
};

// got this from tutorial
class BTSequence : public BTNode {
public:
    BTSequence(std::vector<std::shared_ptr<BTNode>> children)
        : m_index(0), m_children(std::move(children)) {}

    void writeToJson(json &toSave) override
    {
        toSave[BTKeys::TYPE_KEY] = getNodeType();
        toSave[BTKeys::INDEX_KEY] = m_index;
        for (auto& node : m_children)
        {
            json childJson;
            node->writeToJson(childJson);
            toSave[BTKeys::CHILDREN_KEY].push_back(childJson);
        }
    }

    void setFromJson(const json &saved) override
    {
        m_index = saved[BTKeys::INDEX_KEY];
        if (saved.contains(BTKeys::CHILDREN_KEY))
        {
            for (json child : saved[BTKeys::CHILDREN_KEY])
            {
                std::shared_ptr<BTNode> childNode = createSubclassNode(child[BTKeys::TYPE_KEY]);
                childNode->setFromJson(child);
                m_children.push_back(childNode);
            }
        }
    }

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::SEQUENCE_KEY;
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
            //std::cout << m_index << std::endl;
            ++m_index;
            if (m_index >= m_children.size()) {
                return BTState::Success;
            }
            else {
                const auto& nextChild = m_children[m_index];
                //std::cout << m_index << std::endl;
                assert(nextChild);
                nextChild->init(e);
                return BTState::Running;
            }
        }
        // maybe have a situation for BT::Running?
        
        else {
            return state;
        }
    }

    int m_index;
    std::vector<std::shared_ptr<BTNode>> m_children;
};


// Made to be used in M3/M4, 
class BTSelector : public BTNode {
public:
    BTSelector(std::vector<std::shared_ptr<BTNode>> children)
        : m_index(0), m_children(std::move(children)) {

    }
    virtual void init(ECS::Entity e) override {
        m_index = 0;
        assert(m_index < m_children.size());
        // initialize the first child
        const auto& child = m_children[m_index];
        assert(child);
        child->init(e);
    }
    

    virtual BTState process(ECS::Entity e) override {
        //std::cout << "in selector" << std::endl;
        if (m_index >= m_children.size())
            return BTState::Success;

        // process current child
        const auto& child = m_children[m_index];
        assert(child);
        BTState state = child->process(e);

        // select a new active child and initialize its internal state
        if (state == BTState::Success) {
            //std::cout << "Selector Node has succeeded" << std::endl;
            return BTState::Success;
        }
        else if (state == BTState::Failure) {
            //std::cout << "leaf node has failed, move on to next leaf node" << std::endl;
            m_index++;
            const auto& nextChild = m_children[m_index];
            assert(nextChild);
            nextChild->init(e);
            return BTState::Running;
        }
        else {
            //std::cout << "run the same node" << std::endl;
            return BTState::Running;
        }
        
    }

    void writeToJson(json &toSave) override
    {
        toSave[BTKeys::TYPE_KEY] = getNodeType();
        toSave[BTKeys::INDEX_KEY] = m_index;
        for (auto& node : m_children)
        {
            json childJson;
            node->writeToJson(childJson);
            toSave[BTKeys::CHILDREN_KEY].push_back(childJson);
        }
    }

    void setFromJson(const json &saved) override
    {
        m_index = saved[BTKeys::INDEX_KEY];
        if (saved.contains(BTKeys::CHILDREN_KEY))
        {
            for (json child : saved[BTKeys::CHILDREN_KEY])
            {
                std::shared_ptr<BTNode> childNode = createSubclassNode(child[BTKeys::TYPE_KEY]);
                childNode->setFromJson(child);
                m_children.push_back(childNode);
            }
        }
    }

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::SELECTOR_KEY;
    }
private:
    int m_index;
    std::vector<std::shared_ptr<BTNode>> m_children;
};

class RepeatForN : public BTNode {
public:
    RepeatForN(std::shared_ptr<BTNode> child, int n) :
    m_child(child), m_iterationsRemaining(n) {}

    virtual void init(ECS::Entity e) override {
        m_child->init(e);
    }

    virtual BTState process(ECS::Entity e) override {
        //std::cout << m_iterationsRemaining << std::endl;
        BTState state = m_child->process(e);
        if (m_iterationsRemaining > 0 && state == BTState::Running) {
            //std::cout << m_iterationsRemaining << std::endl;
            m_iterationsRemaining = m_iterationsRemaining - 1;
            return BTState::Running;
        }
        
        if (m_iterationsRemaining > 0 && state == BTState::Failure) {
            //std::cout << m_iterationsRemaining << std::endl;
            m_iterationsRemaining = m_iterationsRemaining - 10;
            return BTState::Running;
        }
        
        else if (m_iterationsRemaining <= 0 && state != BTState::Success) {
            //std::cout << "repeat for N should be failing now" << std::endl;
            return BTState::Failure;
        }
        else {
            return BTState::Success;
        }
    }

    void writeToJson(json &toSave) override
    {
        toSave[BTKeys::TYPE_KEY] = getNodeType();
        toSave[BTKeys::ITERATIONS_REMAINING_KEY] = m_iterationsRemaining;
        json childJson;
        m_child->writeToJson(childJson);
        toSave[BTKeys::CHILD_KEY] = childJson;
    }

    void setFromJson(const json &saved) override
    {
        m_iterationsRemaining = saved[BTKeys::ITERATIONS_REMAINING_KEY];
        m_child = createSubclassNode(saved[BTKeys::CHILD_KEY][BTKeys::TYPE_KEY]);
        m_child->setFromJson(saved[BTKeys::CHILD_KEY]);
    }

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::REPEAT_FOR_N_KEY;
    }

private:
    std::shared_ptr<BTNode> m_child;
    int m_iterationsRemaining;
};

class IsSnailInRange : public BTNode {
public:
    IsSnailInRange() {}
    void init(ECS::Entity e) override {}
    BTState process(ECS::Entity e) override;
    void writeToJson(json &toSave) override;
    void setFromJson(const json &saved) override {};

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::SNAIL_IN_RANGE_KEY;
    }
};

class LookForSnail : public BTNode {
public: 
    LookForSnail() noexcept {}
    LookForSnail(bool inRange) : m_inRange(inRange) {}
    void writeToJson(json &toSave) override;
    void setFromJson(const json &saved) override;

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::LOOK_FOR_SNAIL_KEY;
    }

private:
    void init(ECS::Entity e) override {}
    BTState process(ECS::Entity e) override;

private:
    bool m_inRange;
};

class FireXShots : public BTNode {
public:
    FireXShots() noexcept {}
    FireXShots(int skip) : m_Skip(skip) {}
    void init(ECS::Entity e) override {}
    BTState process(ECS::Entity e) override;
    void writeToJson(json &toSave) override;
    void setFromJson(const json &saved) override;

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::FIRE_X_SHOTS_KEY;
    }
private:
    int m_Skip;
};

class RandomSelector : public BTNode {
public:
    // chance is % chance of option #1 happening
    RandomSelector(float chance, std::shared_ptr<BTNode> option1, std::shared_ptr<BTNode> option2):
    m_child1(option1),
    m_child2(option2),
    m_chance(chance) {}

    void writeToJson(json &toSave) override
    {
        toSave[BTKeys::TYPE_KEY] = getNodeType();
        toSave[BTKeys::CHANCE_KEY] = m_chance;

        json child1Json;
        m_child1->writeToJson(child1Json);
        toSave[BTKeys::CHILD_1_KEY] = child1Json;

        json child2Json;
        m_child2->writeToJson(child2Json);
        toSave[BTKeys::CHILD_2_KEY] = child2Json;

        json chosenJson;
        m_chosen->writeToJson(chosenJson);
        toSave[BTKeys::CHOSEN_KEY] = chosenJson;
    }

    void setFromJson(const json &saved) override
    {
        m_chance = saved[BTKeys::CHANCE_KEY];

        m_child1 = createSubclassNode(saved[BTKeys::CHILD_1_KEY][BTKeys::TYPE_KEY]);
        m_child1->setFromJson(saved[BTKeys::CHILD_1_KEY]);

        m_child2 = createSubclassNode(saved[BTKeys::CHILD_2_KEY][BTKeys::TYPE_KEY]);
        m_child2->setFromJson(saved[BTKeys::CHILD_2_KEY]);

        m_chosen = createSubclassNode(saved[BTKeys::CHOSEN_KEY][BTKeys::TYPE_KEY]);
        m_chosen->setFromJson(saved[BTKeys::CHOSEN_KEY]);
    }

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::RANDOM_SELECTOR;
    }
private:
    void init(ECS::Entity e) override {
        m_chance = 1 + rand() % 100;
        if (m_chance <= 75) {
            const auto& child = m_child1;
            m_chosen = child;
            child->init(e);
        }
        else {
            const auto& child = m_child2;
            m_chosen = child;
            child->init(e);
        }
    }
    BTState process(ECS::Entity e) override {
        
        BTState state = m_chosen->process(e);
        if (state == BTState::Success) {
             return BTState::Success;
        }
        else {
            return BTState::Running;
        }
        
    }
private:
    std::shared_ptr<BTNode> m_child1;
    std::shared_ptr<BTNode> m_child2;
    std::shared_ptr<BTNode> m_chosen;
    int m_chance;
};

class PredictShot : public BTNode {
public:
    PredictShot() noexcept {}
    void init(ECS::Entity e) override {}
    BTState process(ECS::Entity e) override;
    void writeToJson(json &toSave) override;
    void setFromJson(const json &saved) override {};

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::PREDICT_SHOT_KEY;
    }
private:
    //int m_Shots;
};

class GetToSnail : public BTNode {
public:
    GetToSnail() noexcept {}
    void init(ECS::Entity e) override {}
    BTState process(ECS::Entity e) override;
    void writeToJson(json &toSave) override;
    void setFromJson(const json &saved) override {}

    NodeType getNodeType() override
    {
        return BTKeys::NodeTypes::GET_TO_SNAIL_KEY;
    }
};


class AISystem
{
public:

    static std::string aiPathFindingAlgorithm;
    static bool aiMoved;
    //static std::vector<vec2> birdPath;
	  void step(float elapsed_ms, vec2 window_size_in_game_units);
    void init();
    static std::vector<vec2> shortestPathBFS(vec2 start, vec2 goal, std::string animal);
    static std::vector<vec2> shortestPathAStar(vec2 start, vec2 goal, std::string animal);
    static void sortQueue(std::deque<std::vector<vec2>> &frontier, vec2 destCoord);
    static bool checkIfReachedDestinationOrAddNeighboringNodesToFrontier(std::deque<std::vector<vec2>>& frontier, std::vector<vec2>& current, TileSystem::vec2Map& tileMovesMap, vec2& goal);
    static bool birdAddNeighborNodes(std::deque<std::vector<vec2>>& frontier, std::vector<vec2>& current, TileSystem::vec2Map& tileMovesMap, vec2& goal);
   // static std::vector<vec2> getBirdPath() { return birdPath; }
};
