// Header
#include "level_loader.hpp"
#include "snail.hpp"
#include "spider.hpp"
#include "tiles/vine.hpp"
#include "tiles/water.hpp"
#include "tiles/wall.hpp"

// stlib
#include <string.h>
#include <fstream>
#include <vector>
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

void LevelLoader::loadLevel(std::string levelFileName)
{
	std::ifstream i(levels_path(levelFileName));
	json level = json::parse(i);

	// get level scale
	float scale = level["scale"];
	TileSystem::setScale(scale);

	// load tile types row by row
	std::string row;
	auto& tiles = TileSystem::getTiles();
	for (int y = 0; y < level["tiles"].size(); y++)
	{
		row = level["tiles"][y];
		int x = 0;
		std::vector<TileSystem::Tile> tileRow;
		for (char const& c : row)
		{
			TileSystem::Tile tile;

			tile.x = x * scale;
			tile.y = y * scale;
			tile.x = (float) (x * scale + 0.5 * scale);
			tile.y = (float) (y * scale + 0.5 * scale);

			switch (c)
			{
			case 'X':
				tile.type = TileSystem::WALL;
				WallTile::createWallTile({ tile.x, tile.y });
				break;
			case 'W':
				tile.type = TileSystem::WATER;
				WaterTile::createWaterTile({ tile.x, tile.y });
				break;
			case 'V':
				tile.type = TileSystem::VINE;
				VineTile::createVineTile({ tile.x, tile.y });
				break;
			default:
				tile.type = TileSystem::EMPTY;
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
		switch (hashit(it.key()))
		{
		case eSnail:
			for (auto &snail : it.value())
			{
				TileSystem::Tile& tile = tiles[snail["y"]][snail["x"]];
				// may not want this for snail location depending on enemy type and AI
				tile.occupied = true;
				Snail::createSnail({ tile.x, tile.y });
			}
			break;
		case eSpider:
			for (auto& spider : it.value())
			{
				TileSystem::Tile& tile = tiles[spider["y"]][spider["x"]];
				tile.occupied = true;
				Spider::createSpider({ tile.x, tile.y });
			}
			break;
		default:
			throw std::runtime_error("Failed to spawn character " + it.key());
			break;
		}
	}
}

// to allow switching on strings
// author: D.Shawley
// source: https://stackoverflow.com/questions/650162/why-the-switch-statement-cannot-be-applied-on-strings
LevelLoader::string_code LevelLoader::hashit(std::string const& inString) {
	if (inString == "snail") return eSnail;
	if (inString == "spider") return eSpider;
	throw std::runtime_error("No hash found for " + inString);
}
