# pragma once

// internal
#include "common.hpp"
#include "tiles/tiles.hpp"
#include "tiny_ecs.hpp"

class LevelLoader
{
public:
	static float previewScale;
	static vec2 previewDimensions;

	static void loadLevel(int levelIndex, bool preview = false, vec2 offset = { 0, 0 }, bool fromSave = false);
	static void previewLevel(int levelIndex, vec2 offset);
private:
	// to allow switching on strings
	// author: D.Shawley
	// source: https://stackoverflow.com/questions/650162/why-the-switch-statement-cannot-be-applied-on-strings
	enum string_code {
		eSnail,
		eSpider,
		eSlug,
		eFish,
		eBird
	};
	static string_code hashit(std::string const& inString);

	// helper for whether a character is within the preview area
	static bool xNotInPreviewArea(int xPos, vec2 previewOrigin);
	static bool yNotInPreviewArea(int yPos, vec2 previewOrigin);


	// creates an entity, tagged with LevelSelectTag if loading a preview
	static ECS::Entity createTaggedEntity(bool preview);
};