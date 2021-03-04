# pragma once

// internal
#include "common.hpp"
#include "tiles/tiles.hpp"

class LevelLoader
{
public:
	static float previewScale;
	static vec2 previewDimensions;

	static void loadLevel(std::string levelFileName, bool preview = false, vec2 offset = { 0, 0 });
	static void previewLevel(std::string levelFileName, vec2 offset);
private:
	// to allow switching on strings
	// author: D.Shawley
	// source: https://stackoverflow.com/questions/650162/why-the-switch-statement-cannot-be-applied-on-strings
	enum string_code {
		eSnail,
		eSpider
	};
	static string_code hashit(std::string const& inString);

	// helper for whether a character is within the preview area
	static bool xNotInPreviewArea(int xPos, vec2 previewOrigin);
	static bool yNotInPreviewArea(int yPos, vec2 previewOrigin);
};