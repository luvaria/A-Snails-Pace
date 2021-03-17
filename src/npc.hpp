#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "tiles/tiles.hpp"
#include "observer.hpp"
#include "subject.hpp"

typedef int CID;
typedef std::pair<int, int> CollectiblePath;
struct npcNode
{
	// sequence of lines to display before going to next node
	std::vector<std::string> lines;
	// collectible ID maps to (points gained, nextNode)
	std::unordered_map<CID, CollectiblePath> collectiblePaths;
	// default next node index
	//	if -1, end npc interaction
	int nextNode;
};

// for now, our only NPC is the other snail
struct NPC : public Observer, public Subject
{
	bool isActive = false;
	// all nodes of the loaded encounter
	// index is dID
	std::vector<npcNode> nodes;
	int curNode = 0;
	int curLine = 0;

	// constructor
	NPC(std::vector<npcNode> nodes);

	// NPC is like a tile type and is loaded the same way
	static ECS::Entity createNPC(Tile& tile, std::string levelName, ECS::Entity entity = ECS::Entity());
	// Creates all the associated render resources and default transform
	static ECS::Entity createNPC(vec2 pos, std::string levelName, ECS::Entity entity = ECS::Entity());

	// loads NPC's nodes from XML file
	static std::vector<npcNode> loadNodes(std::string fileName);

	void beginEncounter();
	void stepEncounter();
	void endEncounter();

	void onNotify(Event event);
};
