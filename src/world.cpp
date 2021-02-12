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
    auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };

	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
    glfwSetMouseButtonCallback(window, mouse_button_redirect);


    // Might want to enforce having only one camera
    // For now we will just have this
    auto cameraEntity = ECS::Entity();
    ECS::registry<Camera>.emplace(cameraEntity);


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


    // Kill snail if off screen
    auto& snailMotion = ECS::registry<Motion>.get(player_snail);
    if (!ECS::registry<DeathTimer>.has(player_snail)
        && offScreen(snailMotion.position, window_size_in_game_units, ECS::registry<Camera>.components[0].offset))
    {
        ECS::registry<DeathTimer>.emplace(player_snail);
        Mix_PlayChannel(-1, salmon_dead_sound, 0);
    }

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

  
    // Reset Camera
    ECS::registry<Camera>.components[0].offset = { 0.f, 0.f };


	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, spiders, ... but that would be more cumbersome
	while (ECS::registry<Motion>.entities.size()>0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();

	// Load level from data/levels
	loadLevel("demo.txt");
//    loadLevel("demo copy.txt");

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
			if (ECS::registry<Spider>.has(entity_other) || ECS::registry<WaterTile>.has(entity_other))
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

bool WorldSystem::offScreen(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset)
{

    vec2 offsetPos = { pos.x - cameraOffset.x, pos.y - cameraOffset.y };


    return (offsetPos.x < 0.f || offsetPos.x > window_size_in_game_units.x
        || offsetPos.y < 0.f || offsetPos.y > window_size_in_game_units.y);
}


void WorldSystem::doX(Motion &motion, WorldSystem::Tile &currTile, WorldSystem::Tile &nextTile, int defaultDirection ) {
    if(currTile.x == nextTile.x) {
        switch (defaultDirection) {
            case DIRECTION_SOUTH:
            case DIRECTION_NORTH:
                if(motion.lastDirection != defaultDirection) {
                    motion.scale.y = -motion.scale.y;
                }
                break;
            default:
                if(motion.lastDirection != defaultDirection) {
                    motion.scale.x = -motion.scale.x;
                }
                break;
        }
    } else if(currTile.x > nextTile.x) {
        if(motion.lastDirection != DIRECTION_WEST) {
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_WEST;
        }
    } else {
        if(motion.lastDirection != DIRECTION_EAST) {
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_EAST;
        }
    }
}

void WorldSystem::doY(Motion &motion, WorldSystem::Tile &currTile, WorldSystem::Tile &nextTile) {
    if(currTile.y == nextTile.y) {
        // nothing
    } else if(currTile.y > nextTile.y) {
        if(motion.lastDirection != DIRECTION_NORTH) {
            motion.scale.y = -motion.scale.y;
            motion.lastDirection = DIRECTION_NORTH;
        }
    } else {
        if(motion.lastDirection != DIRECTION_SOUTH) {
            motion.scale.y = -motion.scale.y;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
}

void WorldSystem::rotate(WorldSystem::Tile &currTile, Motion &motion, WorldSystem::Tile &nextTile) {
    if(abs(currTile.x - nextTile.x) > 0 && abs(currTile.y - nextTile.y) > 0) {
        motion.scale = {motion.scale.y, motion.scale.x};
        if(abs(motion.angle) == PI/2) {
            motion.angle = motion.lastDirection == DIRECTION_NORTH ? 0 : 2*motion.angle;
        } else if (motion.angle == 0) {
            motion.angle = (currTile.x < nextTile.x) ? PI/2 : -PI/2;
        } else {
            motion.angle = (currTile.x < nextTile.x) ? PI/2 : -PI/2;
        }
        motion.lastDirection = abs(motion.angle) == PI/2 ? ((currTile.y > nextTile.y) ? DIRECTION_NORTH : DIRECTION_SOUTH)
                                                         : ((currTile.x > nextTile.x) ? DIRECTION_WEST : DIRECTION_EAST);
    }
}

void WorldSystem::changeDirection(Motion &motion, WorldSystem::Tile &currTile, WorldSystem::Tile &nextTile, int defaultDirection) {
    if(defaultDirection == DIRECTION_SOUTH || defaultDirection == DIRECTION_NORTH) {
        doY(motion, currTile, nextTile);
        rotate(currTile, motion, nextTile);
        doX(motion, currTile, nextTile, defaultDirection);
    } else {
        doX(motion, currTile, nextTile, defaultDirection);
        rotate(currTile, motion, nextTile);
        doY(motion, currTile, nextTile);
    }
    motion.position = {nextTile.x, nextTile.y};
}

void WorldSystem::goLeft(ECS::Entity &entity, int &snail_move) {
    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if(xCoord-1 < 0) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile leftTile = tiles[yCoord][xCoord-1];
    if (abs(motion.angle) != PI/2 && (leftTile.type == GROUND || leftTile.type == WALL)) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.lastDirection = motion.angle == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH;
            motion.angle = PI/2;
        }
        snail_move--;
    }
    else if(abs(motion.angle) != PI/2) {
        int yCord = (motion.angle == -PI/2 ? yCoord+1 : yCoord-1);
        if(yCord < 0 && yCord > tiles.size()-1) {
            return;
        }
        Tile nextTile = tiles[abs(motion.angle) == PI ? (yCoord-1) : (yCoord+1)][xCoord-1];
        nextTile = nextTile.type == GROUND ? leftTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST);
        snail_move--;
    }
}

//void WorldSystem::goRight(ECS::Entity &entity, int &snail_move) {
//    auto& mot = ECS::registry<Motion>.get(entity);
//    int xCoord = static_cast<int>(mot.position.x / scale);
//    int yCoord = static_cast<int>(mot.position.y / scale);
//    bool isSafeMove = false;
//    Tile nextTile;
//    if(abs(mot.angle) == 0 && mot.scale.x < 0) {
//        mot.scale.x = -mot.scale.x;
//    } else if(abs(mot.angle) == PI && mot.scale.x > 0) {
//        mot.scale.x = -mot.scale.x;
//    }
//    if (tiles[yCoord][xCoord+1].type == GROUND || tiles[yCoord][xCoord+1].type == WALL) {
//        if(mot.angle != -PI && mot.scale.y > 0) {
//            mot.scale.y = -mot.scale.y;
//        }
//        if(mot.angle == -PI && mot.scale.y < 0) {
//            mot.scale.y = - mot.scale.y;
//        }
//        if(mot.angle == -PI && mot.scale.x < 0) {
//            mot.scale.x = -mot.scale.x;
//        }
//        mot.angle = PI/2;
//    }
//    else if(mot.angle != PI/2) {
//        nextTile = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord+1];
//        isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE || nextTile.type == WATER;
//        if (isSafeMove && (tiles[yCoord][xCoord+1].type == EMPTY || tiles[yCoord][xCoord+1].type == VINE)) {
//            if(nextTile.type == EMPTY) {
//                Tile& t = tiles[mot.angle == -PI ? (yCoord-1) : (yCoord+1)][xCoord + 1];
//                mot.position = { t.x, t.y };
//                if(abs(mot.angle) == 0 && mot.scale.y < 0) {
//                    mot.scale.y = -mot.scale.y;
//                }
//                if(abs(mot.angle) == 0 && mot.scale.x > 0) {
//                    mot.scale.x = -mot.scale.x;
//                }
//                mot.angle = PI/2;
//            } else if (nextTile.type == WATER) {
//                Tile& t = tiles[yCoord+1][xCoord + 1];
//                mot.position = { t.x, t.y };
//            }
//            else {
//                Tile& t = tiles[yCoord][xCoord + 1];
//                mot.position = { t.x, t.y };
//            }
//            snail_move--;
//        }
//    }
//}

void WorldSystem::goRight(ECS::Entity &entity, int &snail_move) {
    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if(xCoord+1 > tiles[yCoord].size()-1) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile rightTile = tiles[yCoord][xCoord+1];
    if (abs(motion.angle) != PI/2 && (rightTile.type == GROUND || rightTile.type == WALL)) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.lastDirection = motion.angle == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH;
            motion.angle = -PI/2;
        }
        snail_move--;
    }
    else if(abs(motion.angle) != PI/2) {
        int yCord = abs(motion.angle) == PI ? (yCoord-1) : (yCoord+1);
        if(yCord < 0 && yCord > tiles.size()-1) {
            return;
        }
        if(xCoord+1 > tiles[yCord].size()-1) {
            return;
        }
        Tile nextTile = tiles[abs(motion.angle) == PI ? (yCoord-1) : (yCoord+1)][xCoord+1];
        nextTile = nextTile.type == GROUND ? rightTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST);
        snail_move--;
    }
}

void WorldSystem::goUp(ECS::Entity &entity, int &snail_move) {
    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if(yCoord-1 < 0) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord-1][xCoord];
    if (currTile.type == VINE && abs(motion.angle) == 0) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.y - nextTile.y) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI/2 : -PI/2 ;
            motion.lastDirection = DIRECTION_NORTH;
        }
        snail_move--;
    }
    else if (upTile.type == GROUND || upTile.type == WALL) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.lastDirection = motion.angle == PI/2 ? DIRECTION_EAST : DIRECTION_WEST;
            motion.angle = -PI;
        }
        snail_move--;
    }
    else if(abs(motion.angle) == PI/2) {
        
        int xCord = (motion.angle == -PI/2 ? xCoord+1 : xCoord-1);
        if(xCord > tiles[(yCoord-1)].size()-1) {
            return;
        }
        Tile nextTile = tiles[(yCoord-1)][motion.angle == -PI/2 ? xCoord+1 : xCoord-1];
        nextTile = nextTile.type == GROUND || upTile.type == VINE ? upTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH);
        snail_move--;
    }
}


//
//void WorldSystem::goUp(ECS::Entity &entity, int &snail_move) {
//    auto& mot = ECS::registry<Motion>.get(entity);
//    int xCoord = static_cast<int>(mot.position.x / scale);
//    int yCoord = static_cast<int>(mot.position.y / scale);
//
//    bool isSafeMove = false;
//    Tile nextTile;
//    if(mot.scale.y > 0) {
//        mot.scale.y = - mot.scale.y;
//    }
//    if (tiles[yCoord - 1][xCoord].type == VINE) {
//        Tile& t = tiles[mot.angle == 0 ? yCoord : yCoord - 1][xCoord];
//        mot.position = { t.x, t.y };
//        mot.angle = PI/2;
//        snail_move--;
//    }
//    else if (tiles[yCoord - 1][xCoord].type == GROUND || tiles[yCoord - 1][xCoord].type == WALL) {
//        if(mot.scale.y > 0) {
//            mot.scale.y = -mot.scale.y;
//        }
//        mot.angle = -PI;
//    }
//    else if(mot.angle == PI/2) {
//        nextTile = tiles[yCoord-1][mot.scale.x > 0 ? xCoord+1 : xCoord-1];
//        isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
//        if (isSafeMove && (tiles[yCoord-1][xCoord].type == EMPTY || tiles[yCoord-1][xCoord].type == VINE)) {
//            if(nextTile.type == EMPTY || nextTile.type == VINE) {
//                Tile& t = nextTile;
//                mot.position = { t.x, t.y };
//                mot.angle = 0;
//            } else {
//                Tile& t = tiles[yCoord-1][xCoord];
//                mot.position = { t.x, t.y };
//            }
//            snail_move--;
//        }
//    }
//}
void WorldSystem::goDown(ECS::Entity &entity, int &snail_move) {
    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if(yCoord+1 > tiles.size()-1) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord+1][xCoord];
    if (currTile.type == VINE && abs(motion.angle) == PI) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI/2 : -PI/2 ;
            motion.lastDirection = DIRECTION_SOUTH;
        }
        snail_move--;
    } else if (upTile.type == GROUND || upTile.type == WALL) {
        Tile nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = {motion.scale.y, motion.scale.x};
            motion.lastDirection = motion.angle == -PI/2 ? DIRECTION_WEST : DIRECTION_EAST;
            motion.angle = 0;
        }
        snail_move--;
    }
    else if(abs(motion.angle) == PI/2) {
        
        int xCord = (motion.angle == -PI/2 ? xCoord+1 : xCoord-1);
        if(xCord > tiles[(yCoord+1)].size()-1) {
            return;
        }
        Tile nextTile = tiles[(yCoord+1)][motion.angle == -PI/2 ? xCoord+1 : xCoord-1];
        nextTile = nextTile.type == GROUND || upTile.type == VINE ? upTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH);
        snail_move--;
    }
    
}

//void WorldSystem::goDown(ECS::Entity &entity, int &snail_move) {
//    auto& mot = ECS::registry<Motion>.get(entity);
//    int xCoord = static_cast<int>(mot.position.x / scale);
//    int yCoord = static_cast<int>(mot.position.y / scale);
//    bool isSafeMove = false;
//    if (tiles[yCoord + 1][xCoord].type == VINE) {
//        if(mot.scale.y < 0) {
//            mot.scale.y = -mot.scale.y;
//        }
//        Tile& t = tiles[mot.angle == -PI ? yCoord : (yCoord + 1)][xCoord];
//        mot.position = { t.x, t.y };
//        mot.angle = PI/2;
//        snail_move--;
//    } else if(tiles[yCoord + 1][xCoord].type == GROUND) {
//        if(mot.scale.y < 0) {
//            mot.scale.y = -mot.scale.y;
//        }
//        if(mot.scale.y > 0) {
//            mot.scale.y = -mot.scale.y;
//        }
//        mot.scale.x = -mot.scale.x;
//        mot.angle = 0;
//        // snail_move--;
//        // mot.scale = {5, 5}; Squished inside sbehavior
//    }
//    else if(mot.angle == PI/2) {
//        Tile& nextTile = tiles[yCoord+1][mot.scale.x < 0 ? xCoord-1 : xCoord+1];
//        isSafeMove = nextTile.type == GROUND || nextTile.type == EMPTY || nextTile.type == VINE;
//        if (isSafeMove && (tiles[yCoord+1][xCoord].type == EMPTY || tiles[yCoord+1][xCoord].type == VINE)) {
//            if(nextTile.type == EMPTY || nextTile.type == VINE) {
//                Tile& t = nextTile;
//                mot.position = { t.x, t.y };
//                if(mot.scale.y > 0) {
//                    mot.scale.y = -mot.scale.y;
//                }
//                mot.scale.x = -mot.scale.x;
//                mot.angle = -PI;
//            } else {
//                Tile& t = tiles[yCoord+1][xCoord];
//                mot.position = { t.x, t.y };
//            }
//            snail_move--;
//        }
//    }
//}


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
        int tempSnailMove = snail_move;
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_W:
                    goUp(player_snail, snail_move);
				break;
			case GLFW_KEY_S:
                    goDown(player_snail, snail_move);
                break;
			case GLFW_KEY_D:
                    goRight(player_snail, snail_move);
                break;
			case GLFW_KEY_A:
                    goLeft(player_snail, snail_move);
				break;
			}
		}
        
        if (snail_move < tempSnailMove && action == GLFW_PRESS)
        {
            auto& registry = ECS::registry<Motion>;
            for (unsigned int i=0; i< registry.components.size(); i++)
            {
                auto entity = registry.entities[i];
                // For now, we are only interested in collisions that involve the snail
                if (ECS::registry<Spider>.has(entity))
                {
                    int xCoord = static_cast<int>(mot.position.x / scale);
                    int yCoord = static_cast<int>(mot.position.y / scale);
                    
                    auto& motEntity = ECS::registry<Motion>.get(entity);
                    int xCoordEntity = static_cast<int>(motEntity.position.x / scale);
                    int yCoordEntity = static_cast<int>(motEntity.position.y / scale);

//                    vec2 vector = {xCoordEntity-xCoord, yCoordEntity-yCoord};
//                    float magnitude = sqrt(pow(vector[0], 2) + pow(vector[1], 2));
//                    vector[0] /= magnitude;
//                    vector[1] /= magnitude;
//                    float angle = magnitude == 0 ? 0 : atan(vector[1]/vector[0]);

//                    Tile& surfaceBelowAfterTurn = tiles[yCoordEntity + 1][xCoordEntity-xCoord>0 ? xCoordEntity - 1 : xCoordEntity + 1];
                   //Tile& spaceAhead = tiles[yCoordEntity][abs(angle) < PI/2 ? xCoordEntity - 1 : xCoordEntity + 1];
//                    angle = angle * (180/PI);
                    int ai_move=0;
                    int tmp_move = ai_move;
                    if (xCoordEntity-xCoord==0 && yCoordEntity-yCoord==0) {
                    // dont move
                    }else {
                        if (xCoordEntity-xCoord>=0) {
                            goLeft(entity, ai_move);
                            if(ai_move == tmp_move){
                                if(yCoordEntity-yCoord>0) {
                                    goUp(entity, ai_move);
                                } else {
                                    goDown(entity, ai_move);
                                }
                            }
                        }
                        if(ai_move == tmp_move) {
                            goRight(entity, ai_move);
                            if(ai_move == tmp_move){
                                if(yCoordEntity-yCoord>0) {
                                    goUp(entity, ai_move);
                                } else {
                                    goDown(entity, ai_move);
                                }
                            }
                        }
                    }
                }
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

        // Move camera by 1 tile in the x-direction
        // Could eventually move vertically
        // where direction could be loaded from file as per Rebecca's suggestion
        ECS::registry<Camera>.components[0].offset.x += WorldSystem::getScale();

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

void WorldSystem::on_mouse_button(int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Check if we would go off screen and die, and prevent that from happening
        vec2& cameraOffset = ECS::registry<Camera>.components[0].offset;
        vec2 possibleOffset = { cameraOffset.x + WorldSystem::getScale(), cameraOffset.y };
        // Have to get window size this way because no access to window_size_in_game_units
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        if (!offScreen(ECS::registry<Motion>.get(player_snail).position, vec2(w, h), possibleOffset))
        {
            cameraOffset = possibleOffset;
        }
    }
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
                tile.x = x * scale + 0.5 * scale;
                tile.y = y * scale + 0.5 * scale;
				
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
				default:
					tile.type = EMPTY;
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