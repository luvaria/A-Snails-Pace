# pragma once

// internal
#include "common.hpp"
#include "tiles/tiles.hpp"

class LevelLoader
{
public:
	static void loadLevel(std::string levelFileName);
private:
	// to allow switching on strings
	// author: D.Shawley
	// source: https://stackoverflow.com/questions/650162/why-the-switch-statement-cannot-be-applied-on-strings
	enum string_code {
		eSnail,
		eSpider
	};
	static string_code hashit(std::string const& inString);
};
