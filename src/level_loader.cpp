// Header
#include "level_loader.hpp"
#include "snail.hpp"
#include "npc.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "bird.hpp"
#include "fish.hpp"
#include "ai.hpp"
#include "collectible.hpp"
#include "tiles/vine.hpp"
#include "tiles/water.hpp"
#include "tiles/wall.hpp"
#include "menus/level_select.hpp"
#include "load_save.hpp"
#include "projectile.hpp"

// stlib
#include <fstream>
#include <vector>
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>
#include <iostream>

// for convenience
using json = nlohmann::json;

float LevelLoader::previewScale = 20.f;
vec2 LevelLoader::previewDimensions = { 12, 8 };

void LevelLoader::loadLevel(int levelIndex, bool preview, vec2 offset, bool fromSave)
{
	std::ifstream i(levels_path(levels[levelIndex]));
	json level = json::parse(i);

	std::string levelName = level["name"];

	AISystem::aiPathFindingAlgorithm = level["AI-PathFinding-Algorithm"];

	// level scale
	float scale = preview ? previewScale : static_cast<float>(level["scale"]);
	TileSystem::setScale(scale);

	// get level direction
	std::string direction = level["direction"];
	if (direction == "right")
		TileSystem::setScrollDirection(LEFT_TO_RIGHT);
	else if (direction == "down")
		TileSystem::setScrollDirection(TOP_TO_BOTTOM);
	else
		throw std::runtime_error("Could not read level scroll direction.");


	// clear tiles (if previously already loaded a level)
	TileSystem::resetGrid();

    // load camera moves per turn
    unsigned turnsPerCameraMove = level["turnsPerCamera"];
    TileSystem::setTurnsForCameraUpdate(turnsPerCameraMove);

	//load level end coordinates
	ivec2 endCoordinates;
	endCoordinates.x = level["endCoordinates"]["x"];
	endCoordinates.y = level["endCoordinates"]["y"];
	TileSystem::setEndCoordinates(endCoordinates);

	// adjust preview area according to snail spawn location
	// assuming one/first snail
	ivec2 previewOrigin = vec2(level["characters"]["snail"][0]["x"], level["characters"]["snail"][0]["y"]);
	if (preview)
	{
		offset -= static_cast<vec2>(previewOrigin) * previewScale;
		offset.y += previewDimensions.y / 2.f * previewScale; // centre snail vertically
		offset.x += 1.f * previewScale; // show one tile to the left of snail
	}

    json saved;
	json characters;
    json collectibles;
    json npcs;
    if (fromSave)
    {
        std::string const filename = std::string(LoadSaveSystem::LEVEL_DIR) + std::string(LoadSaveSystem::LEVEL_FILE);
        std::ifstream iSaved(save_path(filename));
        saved = json::parse(iSaved);
        characters = saved["characters"];
        collectibles = saved["collectibles"];

        if (saved.contains(LoadSaveSystem::NPC_KEY))
        {
            npcs = saved[LoadSaveSystem::NPC_KEY];
        }
    }
    else
    {
        characters = level["characters"];
        collectibles = level["collectibles"];
    }

	// load tile types row by row
	std::string row;
	auto& tiles = TileSystem::getTiles();

	for (int y = 0; y < level["tiles"].size(); y++)
	{
		// limit preview dimensions (centre snail)
		if (preview && yNotInPreviewArea(y, previewOrigin))
		{
			// do here or else it gets skipped (keeps indices correct for preview)
			std::vector<Tile> tileRow;
			tiles.push_back(tileRow);
			continue;
		}

		row = level["tiles"][y];
		int x = 0;
		std::vector<Tile> tileRow;
		for (char const& c : row)
		{
			// limit preview dimensions (one tile to the left of snail, the rest to the right)
			if (preview && xNotInPreviewArea(x, previewOrigin))
			{
				// do here or else it gets skipped (keeps indices correct for preview)
				Tile tile;
				tileRow.push_back(tile);
				x++;
				continue;
			}

			Tile tile;

			tile.x = (float)(x * scale + 0.5 * scale) + offset.x;
			tile.y = (float)(y * scale + 0.5 * scale) + offset.y;

			auto entity = ECS::Entity();
			if (preview)
				ECS::registry<LevelSelectTag>.emplace(entity);

			switch (c)
			{
			case 'X':
				tile.type = WALL;
				WallTile::createWallTile(tile, entity);
				break;
			case 'W':
				tile.type = WATER;
				WaterTile::createWaterTile(tile, entity);
				break;
			case 'V':
				tile.type = VINE;
				VineTile::createVineTile(tile, entity);
				break;
			case 'N':
			    if (fromSave)
			    {
			        std::string npcPosKey = std::to_string(x) + "," + std::to_string(y);
			        if (!npcs.contains(npcPosKey))
			            break;
			    }

				tile.type = INACCESSIBLE;
				tile.addOccupyingEntity();
				NPC::createNPC(tile, levelName, entity);
				if (fromSave)
                {
				    // perhaps make a function in NPC struct for this lol...
				    NPC& component = ECS::registry<NPC>.get(entity);
                    std::string npcPosKey = std::to_string(x) + "," + std::to_string(y);
                    json savedNPC = npcs[npcPosKey];
				    component.curNode = savedNPC[LoadSaveSystem::NPC_CUR_NODE_KEY];
				    component.curLine = savedNPC[LoadSaveSystem::NPC_CUR_LINE_KEY];
				    component.timesTalkedTo = savedNPC[LoadSaveSystem::NPC_TIMES_TALKED_KEY];
                }
				if (!preview)
				{
					Collectible::equip(entity, 1); // bbcap
				}
				break;
			case 'M':
				tile.type = MESSAGE;
				break;
			default:
				tile.type = EMPTY;
				break;
			}
			tileRow.push_back(tile);
			x++;
		}
		tiles.push_back(tileRow);
	}

    // load characters
	for (auto it = characters.begin(); it != characters.end(); ++it)
	{
		auto entity = ECS::Entity();
		if (preview)
			ECS::registry<LevelSelectTag>.emplace(entity);

		switch (hashit(it.key()))
		{
		case eSnail:
			for (auto& snail : it.value())
			{
				ivec2 snailPos = { snail["x"], snail["y"] };
				if (preview && (xNotInPreviewArea(snailPos.x, previewOrigin) || yNotInPreviewArea(snailPos.y, previewOrigin)))
					continue;
				Tile& tile = tiles[snail["y"]][snail["x"]];
				// may not want this for snail location depending on enemy type and AI
				tile.addOccupyingEntity();
                ECS::Entity snailEntity;
				if (fromSave)
                {
				    Motion motion = LoadSaveSystem::makeMotionFromJson(snail);
				    snailEntity = Snail::createSnail(motion);
                }
				else
                {
                    snailEntity = Snail::createSnail({ tile.x, tile.y }, createTaggedEntity(preview));
                }
                ECS::registry<Player>.emplace(snailEntity);
			}
			break;
		case eSpider:
			for (auto& spider : it.value())
			{
				ivec2 spiderPos = { spider["x"], spider["y"] };
				if (preview && (xNotInPreviewArea(spiderPos.x, previewOrigin) || yNotInPreviewArea(spiderPos.y, previewOrigin)))
					continue;
				Tile& tile = tiles[spider["y"]][spider["x"]];
				tile.addOccupyingEntity();
				if (fromSave)
                {
                    Motion motion = LoadSaveSystem::makeMotionFromJson(spider);
                    std::shared_ptr<BTNode> tree = BTNode::createSubclassNode(spider[LoadSaveSystem::BTREE_KEY][BTKeys::TYPE_KEY]);
                    tree->setFromJson(spider[LoadSaveSystem::BTREE_KEY]);
                    Spider::createSpider(motion, ECS::Entity(), tree);
                }
				else
                {
                    Spider::createSpider({ tile.x, tile.y }, createTaggedEntity(preview));
                }
			}
			break;
		case eSlug:
			for (auto& slug : it.value())
			{
				ivec2 slugPos = { slug["x"], slug["y"] };
				if (preview && (xNotInPreviewArea(slugPos.x, previewOrigin) || yNotInPreviewArea(slugPos.y, previewOrigin)))
					continue;
				Tile& tile = tiles[slug["y"]][slug["x"]];
				tile.addOccupyingEntity();
				if (fromSave)
                {
                    Motion motion = LoadSaveSystem::makeMotionFromJson(slug);
                    std::shared_ptr<BTNode> tree = BTNode::createSubclassNode(slug[LoadSaveSystem::BTREE_KEY][BTKeys::TYPE_KEY]);
                    tree->setFromJson(slug[LoadSaveSystem::BTREE_KEY]);
                    Slug::createSlug(motion, ECS::Entity(), tree);
                }
				else
                {
                    Slug::createSlug({ tile.x, tile.y }, createTaggedEntity(preview));
                }
			}
			break;
		case eFish:
			for (auto& fish : it.value())
			{
				ivec2 fishPos = { fish["x"], fish["y"] };
				if (preview && (xNotInPreviewArea(fishPos.x, previewOrigin) || yNotInPreviewArea(fishPos.y, previewOrigin)))
					continue;
				Tile& tile = tiles[fish["y"]][fish["x"]];
				tile.addOccupyingEntity();
				if (fromSave)
                {
                    Motion motion = LoadSaveSystem::makeMotionFromJson(fish);
                    ECS::Entity entity = Fish::createFish(motion);
                    ECS::registry<Fish::Move>.get(entity).setFromJson(fish[LoadSaveSystem::FISH_MOVE_KEY]);
                }
				else
                {
                    Fish::createFish({ tile.x, tile.y }, createTaggedEntity(preview));
                }
			}
			break;
		case eBird:
			for (auto& bird : it.value())
			{
				ivec2 birdPos = { bird["x"], bird["y"] };
				if (preview && (xNotInPreviewArea(birdPos.x, previewOrigin) || yNotInPreviewArea(birdPos.y, previewOrigin)))
					continue;
				Tile& tile = tiles[bird["y"]][bird["x"]];
				tile.addOccupyingEntity();
				if (fromSave)
                {
                    Motion motion = LoadSaveSystem::makeMotionFromJson(bird);
                    Bird::createBird(motion);
                }
				else
                {
                    Bird::createBird({ tile.x, tile.y }, createTaggedEntity(preview));
                }
			}
			break;
		default:
			throw std::runtime_error("Failed to spawn character " + it.key());
			break;
		}
	}

    // load saved projectiles
    if (fromSave && saved.contains(LoadSaveSystem::PROJECTILE_KEY))
    {
        for (auto& projectile : saved[LoadSaveSystem::PROJECTILE_KEY])
        {
            Motion motion = LoadSaveSystem::makeMotionFromJson(projectile, false);
            switch (hashit(projectile[LoadSaveSystem::PROJECTILE_TYPE_KEY]))
            {
                case eSnail:
                    SnailProjectile::createProjectile(motion);
                    break;
                case eSpider:
                    throw std::runtime_error("somehow loaded a spider projectile???");
                    break;
                case eSlug:
                    SlugProjectile::createProjectile(motion);
                    break;
                default:
                    throw std::runtime_error("failed to load projectile type: " + std::string(projectile[LoadSaveSystem::PROJECTILE_TYPE_KEY]));
            }
        }
    }

	// no moves map or collectibles if preview
	if (preview)
		return;

    for (auto& collectible : collectibles)
    {
        int id = collectible["id"];
        Tile& tile = tiles[collectible["y"]][collectible["x"]];
        Collectible::createCollectible({ tile.x, tile.y }, id);
    }

	TileSystem::vec2Map& tileMovesMap = TileSystem::getAllTileMovesMap();
    tileMovesMap.clear();
	int y = 0;
	for (auto& rows : tiles) // Iterating over rows
	{
		int x = 0;
		for (auto& elem : rows)
		{
			if (elem.type == WALL) {
				if (y - 1 > 0 && (tiles[y - 1][x].type == VINE || tiles[y - 1][x].type == EMPTY)) {
					auto& elem2 = tiles[y - 1][x];
					tileMovesMap.insert({ {y - 1, x}, elem2 });
				}
				if (x - 1 > 0 && (tiles[y][x - 1].type == VINE || tiles[y][x - 1].type == EMPTY)) {
					auto& elem2 = tiles[y][x - 1];
					tileMovesMap.insert({ {y, x - 1}, elem2 });
				}
				if (y + 1 < tiles.size() && (tiles[y + 1][x].type == VINE || tiles[y + 1][x].type == EMPTY)) {
					auto& elem2 = tiles[y + 1][x];
					tileMovesMap.insert({ {y + 1, x}, elem2 });
				}
				if (x + 1 < tiles[y].size() && (tiles[y][x + 1].type == VINE || tiles[y][x + 1].type == EMPTY))
				{
					auto& elem2 = tiles[y][x + 1];
					tileMovesMap.insert({ {y, x + 1}, elem2 });
				}
			}
			else if (elem.type == VINE) {
				tileMovesMap.insert({ {y, x}, elem });
			}
			x++;
		}
		y++;
	}
}

void LevelLoader::previewLevel(int levelIndex, vec2 offset)
{
	loadLevel(levelIndex, true, offset);
}

// to allow switching on strings
// author: D.Shawley
// source: https://stackoverflow.com/questions/650162/why-the-switch-statement-cannot-be-applied-on-strings
LevelLoader::string_code LevelLoader::hashit(std::string const& inString) {
	if (inString == "snail") return eSnail;
	if (inString == "spider") return eSpider;
	if (inString == "slug") return eSlug;
	if (inString == "fish") return eFish;
	if (inString == "bird") return eBird;
	throw std::runtime_error("No hash found for " + inString);
}

bool LevelLoader::xNotInPreviewArea(int xPos, vec2 previewOrigin)
{
	return !((xPos >= previewOrigin.x - 1) && (xPos < previewOrigin.x + previewDimensions.x - 1));
}

bool LevelLoader::yNotInPreviewArea(int yPos, vec2 previewOrigin)
{
	return !((yPos >= previewOrigin.y - previewDimensions.y / 2) && (yPos < previewOrigin.y + previewDimensions.y / 2));
}

ECS::Entity LevelLoader::createTaggedEntity(bool preview)
{
	auto entity = ECS::Entity();
	if (preview)
		ECS::registry<LevelSelectTag>.emplace(entity);
	return entity;
}
