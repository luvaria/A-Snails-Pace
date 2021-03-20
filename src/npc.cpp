// internal
#include "npc.hpp"
#include "render.hpp"
#include "collectible.hpp"

// ext
#include <../ext/pugixml/src/pugixml.hpp>
#include <iostream>

NPC::NPC(std::vector<npcNode> nodes) : nodes(nodes) {}

ECS::Entity NPC::createNPC(Tile& tile, std::string levelName, ECS::Entity entity)
{
	ECS::registry<Tile>.insert(entity, std::move(tile));
	return createNPC(vec2(tile.x, tile.y), levelName, entity);
}

ECS::Entity NPC::createNPC(vec2 position, std::string levelName, ECS::Entity entity)
{
	std::string key = "npc";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
	{
		resource.mesh.loadFromOBJFile(mesh_path("npc.obj"));
		RenderSystem::createColoredMesh(resource, "snail");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//we use the same entity for min and regular meshes, so you can access either one.
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::CHARACTER);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = resource.mesh.original_size / resource.mesh.original_size.x * TileSystem::getScale();
	motion.scale *= 0.9f;
	motion.scale *= -1; // fix orientation
	motion.lastDirection = DIRECTION_WEST;

	// Create an NPC component, loading its dialogue nodes
	ECS::registry<NPC>.emplace(entity, loadNodes(levelName));

	return entity;
}

std::vector<npcNode> NPC::loadNodes(std::string levelName)
{
	std::vector<npcNode> nodes;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(dialogue_path("dialogue.xml").c_str());

	pugi::xml_node encounter = doc.child("dialogues").child("npcs").find_child_by_attribute("name", "OtherSnail").find_child_by_attribute("level_name", levelName.c_str());
	// XML preserves order of children
	for (pugi::xml_node dialogue : encounter.children("dialogue"))
	{
		npcNode node;

		// load lines
		for (pugi::xml_node line : dialogue.child("lines").children("line"))
		{
			node.lines.push_back(line.text().as_string());
		}

		// load collectible paths
		for (pugi::xml_node collectible : dialogue.child("collectibles").children("collectible"))
		{
			node.collectiblePaths.insert({ collectible.attribute("cID").as_int(), { collectible.attribute("points").as_int(), collectible.text().as_int() } });
		}

		// load default next node (if exists)
		if (bool(dialogue.child("next")))
			node.nextNode = dialogue.child("next").text().as_int();
		else
			node.nextNode = -1;

		nodes.push_back(node);
	}

	return nodes;
}

void NPC::beginEncounter()
{
	isActive = true;
	if (timesTalkedTo > 0)
	{
		curNode = 0;
	}
	curLine = 0;
	stepEncounter();
}

void NPC::stepEncounter()
{
	// check if DialogueSystem has more pages to show
	//	if not, DialogueSystem will notify back to start new dialogue
	notify(Event(Event::NEXT_DIALOGUE));
}

void NPC::endEncounter()
{
	isActive = false;
	notify(Event(Event::END_DIALOGUE));
}

void NPC::onNotify(Event event)
{
	if (!isActive) return;

	if (event.type == Event::NEXT_DIALOGUE)
	{
		// no more dialogue
		if (curNode == -1)
		{
			timesTalkedTo++;
			endEncounter();
			return;
		}

		// npcs should have at least one dialogue node with one line
		npcNode& node = nodes[curNode];
		std::string line = node.lines[curLine];

		notify(Event(Event::START_DIALOGUE, line));

		curLine++;
		if (curLine >= node.lines.size())
		{
			curNode = node.nextNode;
			curLine = 0;

			if (node.collectiblePaths.size() > 0)
			{
				Inventory& inv = ECS::registry<Inventory>.components[0];
				for (const std::pair<CID, CollectiblePath> path : node.collectiblePaths)
				{

                    if (inv.equipped == path.first)
					{
						curNode = path.second.second;
						inv.points += path.second.first;
					}
				}
			}
		}
	}
}
