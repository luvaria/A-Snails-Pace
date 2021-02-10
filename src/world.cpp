// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "spider.hpp"
#include "fish.hpp"
#include "tiles/ground.hpp"
#include "tiles/water.hpp"
#include "tiles/vine.hpp"
#include "pebbles.hpp"
#include "render_components.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>

// Game configuration
// CHANGES: Changed MAX_SPIDERS to 1 for testing purposes
const size_t MAX_SPIDERS = 1;
const size_t MAX_FISH = 5;
const size_t SPIDER_DELAY_MS = 2000;
const size_t FISH_DELAY_MS = 5000;

// Zoom level; dimensions of tiles
static float scale = 0.f;

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
	points(0),
	next_spider_spawn(0.f),
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
	window = glfwCreateWindow(window_size_px.x, window_size_px.y, "A Snail's Pace", nullptr, nullptr);
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
	// Updating window title with moves remaining
	std::stringstream title_ss;
	title_ss << "Moves taken: " << turn_number;
	glfwSetWindowTitle(window, title_ss.str().c_str());
	
	// Removing out of screen entities
	auto& registry = ECS::registry<Motion>;

	// Processing the snail state
	assert(ECS::registry<ScreenState>.components.size() <= 1);
	auto& screen = ECS::registry<ScreenState>.components[0];

	for (auto entity : ECS::registry<DeathTimer>.entities)
	{
		// Progress timer
		auto& counter = ECS::registry<DeathTimer>.get(entity);
		counter.counter_ms -= elapsed_ms;

		// Reduce window brightness if any of the present snails is dying
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
	// All that have a motion, we could also iterate over all fish, spiders, ... but that would be more cumbersome
	while (ECS::registry<Motion>.entities.size()>0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();

	// Load level from data/levels
	loadLevel("demo.txt");

	// Initializing turns and amount of tiles snail can move.
	snail_move = 1;
	turn_number = 1;
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

		// For now, we are only interested in collisions that involve the snail
		if (ECS::registry<Snail>.has(entity))
		{
			// Checking Snail - Spider collisions
			if (ECS::registry<Spider>.has(entity_other))
			{
				// initiate death unless already dying
				if (!ECS::registry<DeathTimer>.has(entity))
				{
					// Scream, reset timer, and make the snail sink
					ECS::registry<DeathTimer>.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);
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

float WorldSystem::getScale() { return scale; }

// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
	// Move snail if alive and has turns remaining
	if (!ECS::registry<DeathTimer>.has(player_snail))
	{
		// NEW: Added motion here. I am assuming some sort of rectangular/square level for now
		// this function might get quite messy as time goes on so maybe we will need a decent 
		// amount of helper functions when we get there.
		auto& mot = ECS::registry<Motion>.get(player_snail);
		// CHANGE: Removed salmonX and salmonY. Salmon's position is tracked by using its Motion
		// Component. Had to divide by 100 since starting screen position is (100, 200) which is 
		// associated with the tile at tiles[1][2]. Added snail_move that tracks how many moves
		// the snail can do this turn.
		int xCoord = static_cast<int>(mot.position.x / scale);
		int yCoord = static_cast<int>(mot.position.y / scale);
        bool isSafeMove=false;
        Tile nextTile;
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_W:
                if(mot.scale.y > 0) {
                    mot.scale.y = - mot.scale.y;
                }
				if (tiles[yCoord - 1][xCoord].type == VINE) {
					Tile& t = tiles[mot.angle == 0 ? yCoord : yCoord - 1][xCoord];
					mot.position = { t.x, t.y };
                    mot.angle = PI/2;
					snail_move--;
				}
                else if (tiles[yCoord - 1][xCoord].type == GROUND || tiles[yCoord - 1][xCoord].type == WALL) {
                    if(mot.scale.y > 0) {
                        mot.scale.y = - mot.scale.y;
                    }
                    mot.angle = -PI;
                }
                else if(mot.angle == PI/2) {
                    nextTile = tiles[yCoord-1][mot.scale.x > 0 ? xCoord+1 : xCoord-1];
                    isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
                    if (isSafeMove && (tiles[yCoord-1][xCoord].type == EMPTY || tiles[yCoord-1][xCoord].type == VINE)) {
                        if(nextTile.type == EMPTY || nextTile.type == VINE) {
                            Tile& t = nextTile;
                            mot.position = { t.x, t.y };
                            mot.angle = 0;
                        } else {
                            Tile& t = tiles[yCoord-1][xCoord];
                            mot.position = { t.x, t.y };
                        }
                        snail_move--;
                    }
                }
				break;
			case GLFW_KEY_S:
                if(mot.scale.y < 0) {
                    mot.scale.y = -mot.scale.y;
                }
                if (tiles[yCoord + 1][xCoord].type == VINE) {
					Tile& t = tiles[mot.angle == -PI ? yCoord : (yCoord + 1)][xCoord];
					mot.position = { t.x, t.y };
                    mot.angle = PI/2;
                    snail_move--;
                } else if(tiles[yCoord + 1][xCoord].type == GROUND) {
                    if(mot.scale.y > 0) {
                        mot.scale.y = - mot.scale.y;
                    }
                    mot.angle = 0;
                    snail_move--;
                    // mot.scale = {5, 5}; Squished inside behavior
                }
                else if(mot.angle == PI/2) {
                    nextTile = tiles[yCoord+1][mot.scale.x < 0 ? xCoord-1 : xCoord+1];
                    isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
                    if (isSafeMove && (tiles[yCoord+1][xCoord].type == EMPTY || tiles[yCoord+1][xCoord].type == VINE)) {
                        if(nextTile.type == EMPTY || nextTile.type == VINE) {
                            Tile& t = nextTile;
                            mot.position = { t.x, t.y };
                            mot.angle = -PI;
                            if(mot.scale.y > 0) {
                                mot.scale.y = -mot.scale.y;
                            }
                            if(mot.scale.x < 0) {
                                mot.scale.x = -mot.scale.x;
                            }
                        } else {
                            Tile& t = tiles[yCoord+1][xCoord];
                            mot.position = { t.x, t.y };
                        }
                        snail_move--;
                    }
                }
                
				break;
			case GLFW_KEY_D:
                if(abs(mot.angle) == 0 && mot.scale.x < 0) {
                    mot.scale.x = -mot.scale.x;
                } else if(abs(mot.angle) == PI && mot.scale.x > 0) {
                    mot.scale.x = -mot.scale.x;
                }
                if(mot.angle != PI/2) {
                    nextTile = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord+1];
                    isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
                    if (isSafeMove && (tiles[yCoord][xCoord+1].type == EMPTY || tiles[yCoord][xCoord+1].type == VINE)) {
                        if(nextTile.type == EMPTY) {
                            Tile& t = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord + 1];
                            mot.position = { t.x, t.y };
                            if(abs(mot.angle) == 0 && mot.scale.y < 0) {
                                mot.scale.y = -mot.scale.y;
                            }
                            if(abs(mot.angle) == 0 && mot.scale.x > 0) {
                                mot.scale.x = -mot.scale.x;
                            }
                            mot.angle = PI/2;
                        } else {
                            Tile& t = tiles[yCoord][xCoord + 1];
                            mot.position = { t.x, t.y };
                        }
                        snail_move--;
                    }
                }
				break;
			case GLFW_KEY_A:
                if(abs(mot.angle) == 0 && mot.scale.x > 0) {
                    mot.scale.x = -mot.scale.x;
                } else if(abs(mot.angle) == PI && mot.scale.x < 0) {
                    mot.scale.x = -mot.scale.x;
                }
                if(mot.angle != PI/2) {
                    nextTile = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord-1];
                    isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
                    if (isSafeMove && (tiles[yCoord][xCoord-1].type == EMPTY || tiles[yCoord][xCoord-1].type == VINE || tiles[yCoord][xCoord-1].type == UNUSED)) {
                        if(nextTile.type == EMPTY) {
                            Tile& t = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord - 1];
                            mot.position = { t.x, t.y };
                            if(abs(mot.angle) == 0 && mot.scale.y < 0) {
                                mot.scale.y = -mot.scale.y;
                            }
                            if(abs(mot.angle) == 0 && mot.scale.x < 0) {
                                mot.scale.x = -mot.scale.x;
                            }
                            mot.angle = PI/2;
                        } else {
                            Tile& t = tiles[yCoord][xCoord - 1];
                            mot.position = { t.x, t.y };
                        }
                        snail_move--;
                    }
                }
				break;
			}
		}
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		
		restart();
	}

	// NEW: Next turn button. Might be temporary since we will probably have 
	// a Button Component in the future, but this is just to get the delay
	// agnostic requirement to work.

	if (key == GLFW_KEY_N && action == GLFW_RELEASE) {
		// flip to next turn, reset movement available, make enemies move?.
		turn_number++;
		// Can be more than 1 tile per turn. Will probably gain a different amount
		// depending on snail's status
		snail_move = 1;
	}


	// Debugging
	// CHANGE: Switched debug key to V so it would not trigger when moving to the right
	if (key == GLFW_KEY_V)
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
	if (!ECS::registry<DeathTimer>.has(player_snail))
	{
		(void)mouse_pos;
	}
}

void WorldSystem::loadLevel(std::string level)
{
	std::string line;
	std::ifstream file;
	file.open(levels_path(level), std::ios::in);
	if (file.is_open())
	{
		// get level scale
		std::getline(file, line);
		scale = stof(line);

		// load tile types row by row
		int y = 0;
		while (std::getline(file, line))
		{
			int x = 0;
			std::vector<Tile> tileRow;
			for (char const& c : line)
			{
				Tile tile;
				tile.x = x * scale;
				tile.y = y * scale;
				
				switch (c)
				{
				case 'G':
					tile.type = GROUND;
					GroundTile::createGroundTile({ tile.x, tile.y });
					break;
				case 'W':
					tile.type = WATER;
					WaterTile::createWaterTile({ tile.x, tile.y });
					break;
				case 'V':
					tile.type = VINE;
					VineTile::createVineTile({ tile.x, tile.y });
					break;
				case 'S':
					tile.type = EMPTY;
					player_snail = Snail::createSnail({ tile.x, tile.y });
					break;
				case 'P':
					tile.type = EMPTY;
					Spider::createSpider({ tile.x, tile.y });
					break;
				case ' ':
					tile.type = EMPTY;
					break;
				default:
					tile.type = UNUSED;
					break;
				}
				tileRow.push_back(tile);
				x++;
			}
			tiles.push_back(tileRow);
			y++;
		}
	}
}
