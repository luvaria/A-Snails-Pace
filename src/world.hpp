#pragma once

// internal
#include "common.hpp"
#include "snail.hpp"
#include "tiles/tiles.hpp"
#include "observer.hpp"
#include "subject.hpp"

// stlib
#include <vector>
#include <random>
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

// Container for all our entities and game logic. Individual rendering / update is 
// deferred to the relative update() methods

class WorldSystem : public Observer, public Subject
{

public:

	// Creates a window
	WorldSystem(ivec2 window_size_px);

	// Releases all associated resources
	~WorldSystem();

	// restart level
	void restart(std::string level);

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Should the game be over ?
	bool is_over() const;

	void shootProjectile(vec2 mouse_pos, bool preview = false);

	void onNotify(Event event);
    
    static int snailMoves;

	// return true if a given point is off screen, false otherwise
	static bool offScreen(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset);

	// OpenGL window handle
	GLFWwindow* window;

	// Game running or paused
	bool running;

	// movement functions
	static void goRight(ECS::Entity& entity, int& snail_move);
	static void goLeft(ECS::Entity& entity, int& snail_move);
	static void goUp(ECS::Entity& entity, int& snail_move);
	static void goDown(ECS::Entity& entity, int& snail_move);

	static void fallDown(ECS::Entity& entity, int& snail_move);

	static void doX(Motion& motion, Tile& currTile, Tile& nextTile, int defaultDirection);
	static void doY(Motion& motion, Tile& currTile, Tile& nextTile);

	static void rotate(Tile& currTile, Motion& motion, Tile& nextTile);

	static void changeDirection(Motion& motion, Tile& currTile, Tile& nextTile, int defaultDirection, ECS::Entity& entity);

private:
    // Input callback functions
	void setGLFWCallbacks();
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);
	void on_mouse_button(int button, int action, int /*mods*/);

	// Loads the audio
	void init_audio();

	// Number of fish eaten by the snail, displayed in the window title
	unsigned int points;

	// Game state
	float current_speed;
	float next_spider_spawn;
	float next_fish_spawn;
	ECS::Entity player_snail;
	int attempts;

    std::chrono::time_point<std::chrono::high_resolution_clock> can_show_projectile_preview_time;
	bool left_mouse_pressed;

	// NEW: turn_number is not used for now, but will probably be used to keep track
	// of what day it is on the calendar. snail_move is how many tiles the snail can
	// move during the current turn.

	int turn_number;
	
	// music references
	Mix_Music* background_music;
	Mix_Chunk* salmon_dead_sound;
	Mix_Chunk* salmon_eat_sound;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
