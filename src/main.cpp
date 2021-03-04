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
	RenderSystem renderer(*world.window);
	PhysicsSystem physics;
	AISystem ai;

	// Set all states to default
	TileSystem::setScale(100.f);
	menus.start();
	auto t = Clock::now();
	// Variable timestep loop

	// add the world system as an observer of the menu system to notify when to start the game
	menus.addObserver(&world);
	// add the menu system as an observer of the world to pause and unpause
	world.addObserver(&menus);
	// add the world system as an observer of the physics systems to handle collisions
	physics.addObserver(&world);

	bool addAI = false;

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

			// TRY TO SEE IF THIS CAN BE REPLACED WITH AN OBSERVER PATTERN
			while (addAI == false) {
				auto& aiRegistry = ECS::registry<AI>;
				//std::cout << "here" << std::endl;
				//std::cout << aiRegistry.components.size() << std::endl;
				std::shared_ptr <BTNode> lfs = std::make_unique<LookForSnail>();
				std::shared_ptr <BTNode> tree = std::make_unique<BTSequence>(std::vector<std::shared_ptr <BTNode>>({ lfs }));
				for (unsigned int i = 0; i < aiRegistry.components.size(); i++) {
					// initialize behaviour trees
					tree->init(aiRegistry.entities[i]);
				}
				addAI = true;
			}
			ai.step(elapsed_ms, window_size_in_game_units);
			world.step(elapsed_ms, window_size_in_game_units);
			physics.step(elapsed_ms, window_size_in_game_units);
		}

		renderer.draw(window_size_in_game_units);
	}

	return EXIT_SUCCESS;
}
