#pragma once

// internal
#include "common.hpp"
#include "snail.hpp"
#include "tiles/tiles.hpp"
#include "observer.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

// Container for all our entities and game logic. Individual rendering / update is 
// deferred to the relative update() methods
class WorldSystem : public Observer
{
public:
	// Creates a window
	WorldSystem(ivec2 window_size_px);

	// Releases all associated resources
	~WorldSystem();

	// restart level
	void restart();

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Renders our scene
	void draw();

	// Should the game be over ?
	bool is_over() const;

	void shootProjectile(vec2 mouse_pos);

	void onNotify(const ECS::Entity& entity, Event event);

	// NEW: Defining what tile types are possible, could be used to render correct tile types
	// and to check if can be moved. EMPTY = nothing is on the tile and you can move to it, 
	// OCCUPIED = something is on the tile, UNUSED = don't render anything/tile is not going
	// to be used

	// return true if a given point is off screen, false otherwise
	static bool offScreen(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset);

	// OpenGL window handle
	GLFWwindow* window;
private:
    void goRight(ECS::Entity &entity, int &snail_move);
    
    void goLeft(ECS::Entity &entity, int &snail_move);
    
    void goUp(ECS::Entity &entity, int &snail_move);
    
    void goDown(ECS::Entity &entity, int &snail_move);
    
    void doX(Motion &motion, TileSystem::Tile &currTile, TileSystem::Tile &nextTile, int defaultDirection );
    
    void doY(Motion &motion, TileSystem::Tile &currTile, TileSystem::Tile &nextTile);
    
    void rotate(TileSystem::Tile &currTile, Motion &motion, TileSystem::Tile &nextTile);
    
    void changeDirection(Motion &motion, TileSystem::Tile &currTile, TileSystem::Tile &nextTile, int defaultDirection, ECS::Entity& entity);

    // Input callback functions
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

	// NEW: turn_number is not used for now, but will probably be used to keep track
	// of what day it is on the calendar. snail_move is how many tiles the snail can
	// move during the current turn.

	int turn_number;
	int snail_move;
	
	// music references
	Mix_Music* background_music;
	Mix_Chunk* salmon_dead_sound;
	Mix_Chunk* salmon_eat_sound;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};
