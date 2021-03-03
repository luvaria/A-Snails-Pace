#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class ControlsOverlay
{
public:
	// prompt to press 'C' for controls
	static void addControlsPrompt();
	static void removeControlsPrompt();
	
	// flips toggle and adds/removes overlay according to new status
	static void toggleControlsOverlay();

	// adds overlay if toggle is on
	static bool addControlsOverlayIfOn(); // also returns status of overlay on/off
	
	// removes overlay regardless of toggle on/off
	static void removeControlsOverlay();

	// turns toggle off (false)
	static void toggleControlsOverlayOff();

private:
	// true when overlay toggled on
	static bool showControlsOverlay;

	// adds a key icon with name overlaid as text
	static ECS::Entity addKeyIcon(std::string name, vec2 pos);

	// adds a description for a control
	static ECS::Entity addControlDesc(std::string desc, vec2 pos);
};

struct ControlPrompt
{
	// empty; tag component
};

