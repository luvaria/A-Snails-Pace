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

// json
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>

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
	void restart(int level, bool fromSave = false);

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Should the game be over ?
	bool is_over() const;

	void shootProjectile(vec2 mouse_pos, bool preview = false);

	void onNotify(Event event);

	void setFromJson(nlohmann::json const& saved);
	void writeToJson(nlohmann::json& toSave);

	// return true if a given point is off screen, false otherwise
	static bool offScreen(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset);
    static bool offScreenExceptNegativeYWithBuffer(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset, int buffer);
    
	// movement functions
    static void goRight(ECS::Entity &entity, int &moves);
    static void goLeft(ECS::Entity &entity, int &moves);
    static void goUp(ECS::Entity &entity, int &moves);
    static void goDown(ECS::Entity &entity, int &moves);
    static void fallDown(ECS::Entity& entity, int& moves);
    
    static void doX(Motion &motion, Tile &currTile, Tile &nextTile, int defaultDirection );
    
    static void doY(Motion &motion, Tile &currTile, Tile &nextTile);
    
    static void rotate(Tile &currTile, Motion &motion, Tile &nextTile);
    
    static void changeDirection(Motion &motion, Tile &currTile, Tile &nextTile, int defaultDirection, ECS::Entity& entity);

	static void fishMove(ECS::Entity &entity, int &moves);
    
	// OpenGL window handle
	GLFWwindow* window;

    bool running;

    static float constexpr k_move_seconds = 0.25f;
    static float constexpr k_projectile_turn_ms = 1000.f;

private:
	// Input callback functions

	void setGLFWCallbacks();
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);
	void on_mouse_button(int button, int action, int /*mods*/);

	// Loads the audio
	void init_audio();

	// NPC encounters
	void stopNPC();
	void stepNPC();

	void saveGame();

	// kill the player :(
	void die();

	// Game state
	float current_speed;
	ECS::Entity player_snail;
	ECS::Entity encountered_npc;

	// Stats (for end screen)
	int deaths;
	int enemies_killed;
	int projectiles_fired;

	std::chrono::time_point<std::chrono::high_resolution_clock> can_show_projectile_preview_time;
	bool left_mouse_pressed;
    // if someone holds the left mouse button for a while we shouldn't release the projectile on release of the button
    bool release_projectile;

    int level;
	int turn_number;
    // snail_move is the number of moves the snail has each turn
    // right now we only allow 1
	int snail_move;
	unsigned turns_per_camera_move;
    float projectile_turn_over_time;

	// music references
	Mix_Music* background_music;
	Mix_Chunk* salmon_dead_sound;
	Mix_Chunk* salmon_eat_sound;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
};