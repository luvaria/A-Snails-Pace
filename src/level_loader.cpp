// Header
#include "level_loader.hpp"
#include "snail.hpp"
#include "spider.hpp"
#include "ai.hpp"
#include "tiles/vine.hpp"
#include "tiles/water.hpp"
#include "tiles/wall.hpp"
#include "menus/level_select.hpp"

// stlib
#include <fstream>
#include <vector>
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>
#include <iostream>

// for convenience
using json = nlohmann::json;

float LevelLoader::previewScale = 20.f;
vec2 LevelLoader::previewDimensions = { 12, 8 };

void LevelLoader::loadLevel(int levelIndex, bool preview, vec2 offset)
{
	std::ifstream i(levels_path(levels[levelIndex]));
	json level = json::parse(i);

	AISystem::aiPathFindingAlgorithm = level["AI-PathFinding-Algorithm"];

	// level scale
	float scale = preview ? previewScale : static_cast<float>(level["scale"]);
	TileSystem::setScale(scale);

	// clear tiles (if previously already loaded a level)
	TileSystem::resetGrid();

    // load camera moves per turn
    unsigned turnsPerCameraMove = level["turnsPerCamera"];
    TileSystem::setTurnsForCameraUpdate(turnsPerCameraMove);

	// adjust preview area according to snail spawn location
	// assuming one/first snail
	ivec2 previewOrigin = vec2(level["characters"]["snail"][0]["x"], level["characters"]["snail"][0]["y"]);
	if (preview)
	{
		offset -= static_cast<vec2>(previewOrigin) * previewScale;
		offset.y += previewDimensions.y / 2.f * previewScale; // centre snail vertically
		offset.x += 1.f * previewScale; // show one tile to the left of snail
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
	for (auto it = level["characters"].begin(); it != level["characters"].end(); ++it)
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
				Snail::createSnail({ tile.x, tile.y }, createTaggedEntity(preview));
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
				Spider::createSpider({ tile.x, tile.y }, createTaggedEntity(preview));
			}
			break;
		default:
			throw std::runtime_error("Failed to spawn character " + it.key());
			break;
		}
	}

	// no moves map if preview
	if (preview)
		return;

	TileSystem::vec2Map& tileMovesMap = TileSystem::getAllTileMovesMap();
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