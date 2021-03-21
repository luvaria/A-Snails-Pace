#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <iostream>
#include <memory>
#include <type_traits>

// internal
#include "common.hpp"
#include "world.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "physics.hpp"
#include "ai.hpp"
#include "debug.hpp"
#include "menus/menus.hpp"
#include "load_save.hpp"
#include "parallax_background.hpp"
#include "dialogue.hpp"

using Clock = std::chrono::high_resolution_clock;

const ivec2 window_size_in_px = { 1200, 800 };
const vec2 window_size_in_game_units = { 1200, 800 };
// Note, here the window will show a width x height part of the game world, measured in px. 
// You could also define a window to show 1.5 x 1 part of your game world, where the aspect ratio depends on your window size.

struct Description {
	std::string name;
	Description(const char* str) : name(str) {};
};

// Entry point
int main()
{
	// Initialize the main systems
	WorldSystem world(window_size_in_px);
	MenuSystem menus(*world.window);
	DialogueSystem dialogue(window_size_in_game_units);
	RenderSystem renderer(*world.window);
	PhysicsSystem physics;
	AISystem ai;
	BackgroundSystem bg(window_size_in_game_units);

	// Set all states to default
	TileSystem::setScale(100.f);
	menus.setup();
	menus.openMenu(Event::START_MENU);
	auto t = Clock::now();
	// Variable timestep loop

	// add the world system as an observer of the menu system to notify when to start the game
	menus.addObserver(&world);
	// add the menu system as an observer of the world to pause and unpause
	world.addObserver(&menus);
	// add the world system as an observer of the physics systems to handle collisions
	physics.addObserver(&world);
	// add the background system as an observer of the world system and menu system
	world.addObserver(&bg);
	menus.addObserver(&bg);
	// add the dialogue system as an observer of the world system to start and navigate dialogue
	world.addObserver(&dialogue);

	LoadSaveSystem::loadPlayerFile();

	while (!world.is_over())
	{
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000.f;
		t = now;

		menus.step(window_size_in_game_units);

		if (world.running)
		{
			DebugSystem::clearDebugComponents();
			ai.step(elapsed_ms, window_size_in_game_units);
			world.step(elapsed_ms, window_size_in_game_units);
			physics.step(elapsed_ms, window_size_in_game_units);
			bg.step();
			dialogue.step(elapsed_ms, window_size_in_game_units);
		}

		renderer.draw(window_size_in_game_units, elapsed_ms);
	}

	LoadSaveSystem::writePlayerFile();

	return EXIT_SUCCESS;
}
