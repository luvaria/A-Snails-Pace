// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "turtle.hpp"
#include "fish.hpp"
#include "pebbles.hpp"
#include "render_components.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>

// Game configuration
const size_t MAX_TURTLES = 15;
const size_t MAX_FISH = 5;
const size_t TURTLE_DELAY_MS = 2000;
const size_t FISH_DELAY_MS = 5000;

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
	points(0),
	next_turtle_spawn(0.f),
	next_fish_spawn(0.f)
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());

	///////////////////////////////////////
	// Initialize GLFW
	auto glfw_err_callback = [](int error, const char* desc) { std::cerr << "OpenGL:" << error << desc << std::endl; };
	glfwSetErrorCallback(glfw_err_callback);
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	//-------------------------------------------------------------------------
	// GLFW / OGL Initialization, needs to be set before glfwCreateWindow
	// Core Opengl 3.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_size_px.x, window_size_px.y, "Salmon Game Assignment", nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error("Failed to glfwCreateWindow");

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	// Playing background music indefinitely
	init_audio();
	Mix_PlayMusic(background_music, -1);
	std::cout << "Loaded music\n";
}

WorldSystem::~WorldSystem(){
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (salmon_dead_sound != nullptr)
		Mix_FreeChunk(salmon_dead_sound);
	if (salmon_eat_sound != nullptr)
		Mix_FreeChunk(salmon_eat_sound);
	Mix_CloseAudio();

	// Destroy all created components
	ECS::ContainerInterface::clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

void WorldSystem::init_audio()
{
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");

	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	salmon_dead_sound = Mix_LoadWAV(audio_path("salmon_dead.wav").c_str());
	salmon_eat_sound = Mix_LoadWAV(audio_path("salmon_eat.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr)
		throw std::runtime_error("Failed to load sounds make sure the data directory is present: "+
			audio_path("music.wav")+
			audio_path("salmon_dead.wav")+
			audio_path("salmon_eat.wav"));

}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());
	
	// Removing out of screen entities
	auto& registry = ECS::registry<Motion>;

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current upon delete)
	for (int i = static_cast<int>(registry.components.size())-1; i >= 0; --i)
	{
		auto& motion = registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f)
		{
			ECS::ContainerInterface::remove_all_components_of(registry.entities[i]);
		}
	}

	// Spawning new turtles
	next_turtle_spawn -= elapsed_ms * current_speed;
	if (ECS::registry<Turtle>.components.size() <= MAX_TURTLES && next_turtle_spawn < 0.f)
	{
		// Reset timer
		next_turtle_spawn = (TURTLE_DELAY_MS / 2) + uniform_dist(rng) * (TURTLE_DELAY_MS / 2);
		// Create turtle
		ECS::Entity entity = Turtle::createTurtle({0, 0});
		// Setting random initial position and constant velocity
		auto& motion = ECS::registry<Motion>.get(entity);
		motion.position = vec2(window_size_in_game_units.x - 150.f, 50.f + uniform_dist(rng) * (window_size_in_game_units.y - 100.f));
		motion.velocity = vec2(100.f, 0.f );
	}

	// Spawning new fish
	next_fish_spawn -= elapsed_ms * current_speed;
	if (ECS::registry<Fish>.components.size() <= MAX_FISH && next_fish_spawn < 0.f)
	{
		// !!! TODO A1: Create new fish with Fish::createFish({0,0}), as for the Turtles above
		if(false) // dummy to silence warning about unused function until implemented
			Fish::createFish({ 0,0 });
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE SPAWN/UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Processing the salmon state
	assert(ECS::registry<ScreenState>.components.size() <= 1);
	auto& screen = ECS::registry<ScreenState>.components[0];

	for (auto entity : ECS::registry<DeathTimer>.entities)
	{
		// Progress timer
		auto& counter = ECS::registry<DeathTimer>.get(entity);
		counter.counter_ms -= elapsed_ms;

		// Reduce window brightness if any of the present salmons is dying
		screen.darken_screen_factor = 1-counter.counter_ms/3000.f;

		// Restart the game once the death timer expired
		if (counter.counter_ms < 0)
		{
			ECS::registry<DeathTimer>.remove(entity);
			screen.darken_screen_factor = 0;
			restart();
			return;
		}
	}

	// !!! TODO A1: update LightUp timers and remove if time drops below zero, similar to the DeathTimer
}

// Reset the world state to its initial state
void WorldSystem::restart()
{
	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();
	std::cout << "Restarting\n";

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
	while (ECS::registry<Motion>.entities.size()>0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();

	// Create a new salmon
	player_salmon = Salmon::createSalmon({ 100, 200 });

	// !! TODO A3: Enable static pebbles on the ground
	/*
	// Create pebbles on the floor
	for (int i = 0; i < 20; i++)
	{
		int w, h;
		glfwGetWindowSize(m_window, &w, &h);
		float radius = 30 * (m_dist(m_rng) + 0.3f); // range 0.3 .. 1.3
		Pebble::createPebble({ m_dist(m_rng) * w, h - m_dist(m_rng) * 20 }, { radius, radius });
	}
	*/
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	auto& registry = ECS::registry<PhysicsSystem::Collision>;
	for (unsigned int i=0; i< registry.components.size(); i++)
	{
		// The entity and its collider
		auto entity = registry.entities[i];
		auto entity_other = registry.components[i].other;

		// For now, we are only interested in collisions that involve the salmon
		if (ECS::registry<Salmon>.has(entity))
		{
			// Checking Salmon - Turtle collisions
			if (ECS::registry<Turtle>.has(entity_other))
			{
				// initiate death unless already dying
				if (!ECS::registry<DeathTimer>.has(entity))
				{
					// Scream, reset timer, and make the salmon sink
					ECS::registry<DeathTimer>.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);

					// !!! TODO A1: change the salmon motion to float down up-side down

					// !!! TODO A1: change the salmon color
				}
			}
			// Checking Salmon - Fish collisions
			else if (ECS::registry<Fish>.has(entity_other))
			{
				if (!ECS::registry<DeathTimer>.has(entity))
				{
					// chew, count points, and set the LightUp timer 
					ECS::ContainerInterface::remove_all_components_of(entity_other);
					Mix_PlayChannel(-1, salmon_eat_sound, 0);
					++points;

					// !!! TODO A1: create a new struct called LightUp in render_components.hpp and add an instance to the salmon entity
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	ECS::registry<PhysicsSystem::Collision>.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
	return glfwWindowShouldClose(window)>0;
}

// On key callback
// TODO A1: check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
	// Move salmon if alive
	if (!ECS::registry<DeathTimer>.has(player_salmon))
	{
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO A1: HANDLE SALMON MOVEMENT HERE
		// key is of 'type' GLFW_KEY_
		// action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		
		restart();
	}

	// Debugging
	if (key == GLFW_KEY_D)
		DebugSystem::in_debug_mode = (action != GLFW_RELEASE);

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA)
	{
		current_speed -= 0.1f;
		std::cout << "Current speed = " << current_speed << std::endl;
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD)
	{
		current_speed += 0.1f;
		std::cout << "Current speed = " << current_speed << std::endl;
	}
	current_speed = std::max(0.f, current_speed);
}

void WorldSystem::on_mouse_move(vec2 mouse_pos)
{
	if (!ECS::registry<DeathTimer>.has(player_salmon))
	{
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO A1: HANDLE SALMON ROTATION HERE
		// xpos and ypos are relative to the top-left of the window, the salmon's 
		// default facing direction is (1, 0)
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		(void)mouse_pos;
	}
}
