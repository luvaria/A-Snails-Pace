// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "spider.hpp"
#include "projectile.hpp"
#include "fish.hpp"
#include "ai.hpp"
#include "tiles/wall.hpp"
#include "tiles/water.hpp"
#include "tiles/vine.hpp"
#include "pebbles.hpp"
#include "render.hpp"
#include "render_components.hpp"
#include "tiles/tiles.hpp"
#include "level_loader.hpp"
#include "controls_overlay.hpp"
#include "observer.hpp"
#include "subject.hpp"

// stlib
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
const float MOVE_S = 0.1f;
const size_t PROJECTILE_PREVIEW_DELAY_MS = 200;

std::string level = "demo.json";

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
    points(0),
    next_spider_spawn(0.f),
    next_fish_spawn(0.f),
    left_mouse_pressed(false),
    attempts(-1),
    running(false)
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

    // Might want to enforce having only one camera
    // For now we will just have this
    auto cameraEntity = ECS::Entity();
    ECS::registry<Camera>.emplace(cameraEntity);
    ECS::registry<Camera>.get(cameraEntity).offset = { 0.f, 0.f };


    // Playing background music indefinitely
    init_audio();
    Mix_PlayMusic(background_music, -1);
    std::cout << "Loaded music\n";
}

WorldSystem::~WorldSystem() {
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
        throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
            audio_path("music.wav") +
            audio_path("salmon_dead.wav") +
            audio_path("salmon_eat.wav"));

}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    // exit early if no game currently running
    if (!running)
        return;

    // Updating window title with moves remaining and attempts
    std::stringstream title_ss;
    title_ss << "Moves taken: " << turn_number;
    title_ss << ", ";
    title_ss << "Attempts: " << attempts;
    glfwSetWindowTitle(window, title_ss.str().c_str());

    // Kill snail if off screen
    auto& snailMotion = ECS::registry<Motion>.get(player_snail);
    if (!ECS::registry<DeathTimer>.has(player_snail)
        && offScreen(snailMotion.position, window_size_in_game_units, ECS::registry<Camera>.components[0].offset))
    {
        ECS::registry<DeathTimer>.emplace(player_snail);
        Mix_PlayChannel(-1, salmon_dead_sound, 0);
    }

    //remove any offscreen projectiles
    for (auto entity : ECS::registry<Projectile>.entities)
    {
        auto projectilePosition = ECS::registry<Motion>.get(entity).position;
        if (offScreen(projectilePosition, window_size_in_game_units, ECS::registry<Camera>.components[0].offset))
        {
            ECS::ContainerInterface::remove_all_components_of(entity);
        }
    }

    if (left_mouse_pressed && (std::chrono::high_resolution_clock::now() > can_show_projectile_preview_time))
    {
        double mouse_pos_x, mouse_pos_y;
        glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
        vec2 mouse_pos = vec2(mouse_pos_x, mouse_pos_y);
        shootProjectile(mouse_pos, true);
        can_show_projectile_preview_time = std::chrono::high_resolution_clock::now()
            + std::chrono::milliseconds{ PROJECTILE_PREVIEW_DELAY_MS };
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
        screen.darken_screen_factor = 1 - counter.counter_ms / 3000.f;

        // Restart the game once the death timer expired
        if (counter.counter_ms < 0)
        {
            ECS::registry<DeathTimer>.remove(entity);
            restart(level);
            return;
        }
    }
}

// Reset the world state to its initial state
void WorldSystem::restart(std::string newLevel)
{
    // Debugging for memory/component leaks
    ECS::ContainerInterface::list_all_components();
    std::cout << "Restarting\n";

    // Reset the game speed
    current_speed = 1.f;

    // Increment attempts
    attempts++;

    // Reset screen darken factor (manual restart can remove DeathTimer without resetting)
    auto& screen = ECS::registry<ScreenState>.components[0];
    screen.darken_screen_factor = 0;

    // Reset Camera
    ECS::registry<Camera>.components[0].offset = { 0.f, 0.f };

    // Remove all entities with Motion, iterating backwards
    for (int i = ECS::registry<Motion>.entities.size() - 1; i >= 0; i--)
    {
        ECS::Entity entity = ECS::registry<Motion>.entities[i];
        // ignore controls overlay entities
        if (!ECS::registry<Overlay>.has(entity))
        {
            ECS::ContainerInterface::remove_all_components_of(entity);
        }
    }

    // Debugging for memory/component leaks
    ECS::ContainerInterface::list_all_components();

    // Load level from data/levels
    level = newLevel;
    LevelLoader::loadLevel(newLevel);
    // can't access player_snail in level loader
    player_snail = ECS::registry<Snail>.entities[0];

    // for the first level, prompt controls overlay
    if (level == "demo.json" && attempts == 0)
        ControlsOverlay::addControlsPrompt();

    // Initializing turns and amount of tiles snail can move.
    turn_number = 1;

    setGLFWCallbacks();
}

void WorldSystem::onNotify(Event event) {

    if (event.type == Event::UNPAUSE)
    {
        setGLFWCallbacks();
        ControlsOverlay::addControlsOverlayIfOn();
        running = true;
    }
    else if (event.type == Event::COLLISION) {

        // Collisions involving snail
        if (ECS::registry<Snail>.has(event.entity))
        {
            // Checking Snail - Spider collisions
            if (ECS::registry<Spider>.has(event.other_entity) || ECS::registry<WaterTile>.has(event.other_entity))
            {
                // initiate death unless already dying
                if (!ECS::registry<DeathTimer>.has(event.entity))
                {
                    // Scream, reset timer, and make the snail sink
                    ECS::registry<DeathTimer>.emplace(event.entity);
                    Mix_PlayChannel(-1, salmon_dead_sound, 0);
                }
            }
        }

        //collisions involving the projectiles
        if (ECS::registry<Projectile>.has(event.entity))
        {
            // Don't collide with a preview projectile (ie. all enemies should fall under here)
            if (!ECS::registry<Projectile::Preview>.has(event.entity))
            {
                // Checking Projectile - Spider collisions
                if (ECS::registry<Spider>.has(event.other_entity))
                {
                    //remove both the spider and the projectile
                    ECS::ContainerInterface::remove_all_components_of(event.entity);
                    ECS::ContainerInterface::remove_all_components_of(event.other_entity);
                }
            }
        }
    }
    else if (event.type == Event::LOAD_LEVEL)
    {
        running = true;
        DebugSystem::in_debug_mode = false;
        attempts = -1;
        ControlsOverlay::toggleControlsOverlayOff();
        switch (event.level)
        {
        case Event::Level::DEMO:
            level = "demo.json";
            break;
        case Event::Level::DEMO_2:
            level = "demo-2.json";
            break;
        default:
            throw std::runtime_error("Could not load unknown level.");
        }
        restart(level);
    }
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
    return glfwWindowShouldClose(window) > 0;
}

bool WorldSystem::offScreen(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset)
{

    vec2 offsetPos = { pos.x - cameraOffset.x, pos.y - cameraOffset.y };


    return (offsetPos.x < 0.f || offsetPos.x > window_size_in_game_units.x
        || offsetPos.y < 0.f || offsetPos.y > window_size_in_game_units.y);
}


void WorldSystem::doX(Motion& motion, Tile& currTile, Tile& nextTile, int defaultDirection) {
    if (currTile.x == nextTile.x) {
        switch (defaultDirection) {
        case DIRECTION_SOUTH:
        case DIRECTION_NORTH:
            if (motion.lastDirection != defaultDirection) {
                motion.scale.y = -motion.scale.y;
            }
            break;
        default:
            if (motion.lastDirection != defaultDirection) {
                motion.scale.x = -motion.scale.x;
            }
            break;
        }
    }
    else if (currTile.x > nextTile.x) {
        if (motion.lastDirection != DIRECTION_WEST) {
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_WEST;
        }
    }
    else {
        if (motion.lastDirection != DIRECTION_EAST) {
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_EAST;
        }
    }
}

void WorldSystem::doY(Motion& motion, Tile& currTile, Tile& nextTile) {
    if (currTile.y == nextTile.y) {
        // nothing
    }
    else if (currTile.y > nextTile.y) {
        if (motion.lastDirection != DIRECTION_NORTH) {
            motion.scale.y = -motion.scale.y;
            motion.lastDirection = DIRECTION_NORTH;
        }
    }
    else {
        if (motion.lastDirection != DIRECTION_SOUTH) {
            motion.scale.y = -motion.scale.y;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
}

void WorldSystem::rotate(Tile& currTile, Motion& motion, Tile& nextTile) {
    if (abs(currTile.x - nextTile.x) > 0 && abs(currTile.y - nextTile.y) > 0) {
        motion.scale = { motion.scale.y, motion.scale.x };
        if (abs(motion.angle) == PI / 2) {
            motion.angle = motion.lastDirection == DIRECTION_NORTH ? 0 : 2 * motion.angle;
        }
        else if (motion.angle == 0) {
            motion.angle = (currTile.x < nextTile.x) ? PI / 2 : -PI / 2;
        }
        else {
            motion.angle = (currTile.x < nextTile.x) ? PI / 2 : -PI / 2;
        }
        motion.lastDirection = abs(motion.angle) == PI / 2 ? ((currTile.y > nextTile.y) ? DIRECTION_NORTH : DIRECTION_SOUTH)
            : ((currTile.x > nextTile.x) ? DIRECTION_WEST : DIRECTION_EAST);
    }
}

void WorldSystem::changeDirection(Motion& motion, Tile& currTile, Tile& nextTile, int defaultDirection, ECS::Entity& entity) {
    if (defaultDirection == DIRECTION_SOUTH || defaultDirection == DIRECTION_NORTH) {
        doY(motion, currTile, nextTile);
        rotate(currTile, motion, nextTile);
        doX(motion, currTile, nextTile, defaultDirection);
    }
    else {
        doX(motion, currTile, nextTile, defaultDirection);
        rotate(currTile, motion, nextTile);
        doY(motion, currTile, nextTile);
    }

    Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
    dest.position = { nextTile.x, nextTile.y };
    // give velocity to reach destination in set time
    // this velocity will be set to 0 once destination is reached in physics.cpp
    motion.velocity = (dest.position - motion.position) / MOVE_S;
}

void WorldSystem::goLeft(ECS::Entity& entity, int& snail_move) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    int cameraOffsetX = ECS::registry<Camera>.components[0].offset.x / TileSystem::getScale();
    if (xCoord - 1 < 0 || xCoord - 1 < cameraOffsetX) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile leftTile = tiles[yCoord][xCoord - 1];
    Tile nextTile = currTile;
    if (abs(motion.angle) != PI / 2 && (leftTile.type == WALL)) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.lastDirection = motion.angle == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH;
            motion.angle = PI / 2;
        }
    }
    else if (abs(motion.angle) != PI / 2) {
        int yCord = (motion.angle == -PI / 2 ? yCoord + 1 : yCoord - 1);
        if (yCord < 0 && yCord > tiles.size() - 1) {
            return;
        }
        nextTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][xCoord - 1];
        Tile sideTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][(xCoord)];
        if (!(nextTile.type == WALL || leftTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
            return;
        }
        nextTile = nextTile.type == WALL || leftTile.type == VINE ? leftTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST, entity);
    }
    else if (abs(motion.angle) == PI / 2 && leftTile.type == VINE) {
        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x;
        motion.lastDirection = DIRECTION_WEST;
        motion.angle = 0;
    }

    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        snail_move--;
    }
}

void WorldSystem::goRight(ECS::Entity& entity, int& snail_move) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if (xCoord + 1 > tiles[yCoord].size() - 1) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile rightTile = tiles[yCoord][xCoord + 1];
    Tile nextTile = currTile;
    if (abs(motion.angle) != PI / 2 && rightTile.type == WALL) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.lastDirection = motion.angle == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH;
            motion.angle = -PI / 2;
        }
    }
    else if (abs(motion.angle) != PI / 2) {
        int yCord = abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1);
        if (yCord < 0 && yCord > tiles.size() - 1) {
            return;
        }
        if (xCoord + 1 > tiles[yCord].size() - 1) {
            return;
        }
        nextTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][xCoord + 1];
        Tile sideTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][(xCoord)];
        if (!(nextTile.type == WALL || rightTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
            return;
        }
        nextTile = nextTile.type == WALL || rightTile.type == VINE ? rightTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST, entity);
    }
    else if (abs(motion.angle) == PI / 2 && rightTile.type == VINE) {
        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x;
        motion.lastDirection = DIRECTION_EAST;
        motion.angle = 0;
    }

    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        snail_move--;
    }
}

void WorldSystem::goUp(ECS::Entity& entity, int& snail_move) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if (yCoord - 1 < 0) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord - 1][xCoord];
    Tile nextTile = currTile;
    if (currTile.type == VINE && abs(motion.angle) == 0) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.y - nextTile.y) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI / 2 : -PI / 2;
            motion.lastDirection = DIRECTION_NORTH;
        }
    }
    else if (upTile.type == WALL) {
        // This is to fix a movement bug where snail flips to weird positions when: a. it is upside down
        // and b. W is pressed. In order to fix that bug I have written this if statement
        // which might not be the best way to do it, but solves the bug for now.
        if (tiles[yCoord - 1][xCoord + 1].type == WALL && (motion.angle == PI || motion.angle == -PI)) {
            return;
        }

        if (tiles[yCoord - 1][xCoord + 1].type != WALL && currTile.type == VINE &&
            (motion.angle == PI || motion.angle == -PI)) {
            /*
            Tile nextTile = tiles[yCoord - 1][xCoord + 1];
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { nextTile.x, nextTile.y };
            motion.velocity = (dest.position - motion.position) / MOVE_S;
            //motion.position.x = (xCoord + 1) * scale + 50;
            //motion.position.y = (yCoord - 1) * scale + 50;
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = PI / 2;
            motion.lastDirection = DIRECTION_NORTH;
            */
            return;
        }

        if (tiles[yCoord - 1][xCoord + 1].type != WALL && (motion.angle == PI || motion.angle == -PI)) {
            return;
        }
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.lastDirection = motion.angle == PI / 2 ? DIRECTION_EAST : DIRECTION_WEST;
            motion.angle = -PI;
        }
    }
    else if (abs(motion.angle) == PI / 2) {

        int xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
        if (xCord > tiles[(yCoord - 1)].size() - 1) {
            return;
        }
        nextTile = tiles[(yCoord - 1)][motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1];
        Tile sideTile = tiles[(yCoord)][motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1];
        if (!(nextTile.type == WALL || upTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
            return;
        }
        nextTile = nextTile.type == WALL || upTile.type == VINE ? upTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH, entity);
    }

    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        snail_move--;
    }
}

void WorldSystem::goDown(ECS::Entity& entity, int& snail_move) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    // it's not like this in the others because right now it's only down where we move multiple spaces per frame (falling)
    auto& motion = ECS::registry<Motion>.get(entity);
    vec2& position = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity).position : motion.position;

    int xCoord = static_cast<int>(position.x / scale);
    int yCoord = static_cast<int>(position.y / scale);
    if (yCoord + 1 > tiles.size() - 1) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord + 1][xCoord];
    Tile nextTile = currTile;
    if (currTile.type == VINE && abs(motion.angle) == PI) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI / 2 : -PI / 2;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
    else if (upTile.type == WALL) {
        nextTile = tiles[yCoord][xCoord];
        if (motion.angle != 0 && abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH, entity);
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.lastDirection = motion.angle == -PI / 2 ? DIRECTION_WEST : DIRECTION_EAST;
            motion.angle = 0;
        }
    }
    else if (currTile.type == VINE && abs(motion.angle) == 0) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.y - nextTile.y) == 0) {
            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? -PI / 2 : PI / 2;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
    else if (abs(motion.angle) == PI / 2) {

        int xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
        if (xCord > tiles[(yCoord + 1)].size() - 1) {
            return;
        }
        nextTile = tiles[(yCoord + 1)][motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1];
        Tile sideTile = tiles[(yCoord)][motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1];
        if (!(nextTile.type == WALL || upTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
            return;
        }
        nextTile = nextTile.type == WALL || upTile.type == VINE ? upTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH, entity);
    }

    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        snail_move--;
    }

}

void WorldSystem::fallDown(ECS::Entity& entity, int& snail_move) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);

    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord + 1][xCoord];
    if (upTile.type == WALL) {
        return;
    }
    for (int i = yCoord + 1; i < tiles.size(); i++) {
        Tile t = tiles[i][xCoord];
        // this part of the code is still slightly buggy. It works fine for first iteration. After snail dies and if pressed
        // with water tile at bottom of fall and snail being upside down, snail will flip over to top of platform
        // instead of falling
        // NVM I think I have fixed it, but I am keeping the message just in case someone else runs into it.
        if (t.type == WATER) {
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { t.x, t.y };
            // give velocity to reach destination in set time
            // this velocity will be set to 0 once destination is reached in physics.cpp
            motion.velocity = (dest.position - motion.position) / MOVE_S;
        }
        else if (t.type == WALL) {
            //std::cout << "here" << std::endl;
            //std::cout << xCoord << ", " << i << std::endl;
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { tiles[i - 1][xCoord].x, tiles[i - 1][xCoord].y };
            // give velocity to reach destination in set time
            // this velocity will be set to 0 once destination is reached in physics.cpp
            motion.velocity = (dest.position - motion.position) / MOVE_S;
            if (abs(motion.angle) == PI / 2) {
                goDown(entity, snail_move);
            }
            else if (motion.angle == -PI) {
                if (motion.lastDirection == DIRECTION_EAST) {
                    motion.scale = { motion.scale.y, motion.scale.x };
                    goDown(entity, snail_move);
                }
                else {
                    motion.scale = { motion.scale.y, -motion.scale.x };
                    goDown(entity, snail_move);
                }
            }
            else if (motion.angle == PI) {
                if (motion.lastDirection == DIRECTION_WEST) {
                    motion.scale = { motion.scale.y, -motion.scale.x };
                    goDown(entity, snail_move);
                }
                else {
                    motion.scale = { motion.scale.y, motion.scale.x };
                    goDown(entity, snail_move);
                }
            }
            i = tiles.size();
        }
    }
    return;
}

// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
    // remove prompt on mouse click
    if (action == GLFW_PRESS)
        ControlsOverlay::removeControlsPrompt();

    // Move snail if alive and has turns remaining
    if (!ECS::registry<DeathTimer>.has(player_snail))
    {
        // NEW: Added motion here. I am assuming some sort of rectangular/square level for now
        // this function might get quite messy as time goes on so maybe we will need a decent 
        // amount of helper functions when we get there.
        // CHANGE: Removed salmonX and salmonY. Salmon's position is tracked by using its Motion
        // Component. Had to divide by 100 since starting screen position is (100, 200) which is 
        // associated with the tile at tiles[1][2]. Added snail_move that tracks how many moves
        // the snail can do this turn.
        int tempSnailMove = 0;
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                // These logs are pretty useful to see what scale, angle and direction the snail should have
            case GLFW_KEY_W:
                goUp(player_snail, tempSnailMove);
                //std::cout << mot.angle << std::endl;
                //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                //std::cout << mot.lastDirection << std::endl;
                //std::cout << motion.position.x << ", " << motion.position.y << std::endl;
                break;
            case GLFW_KEY_S:
                goDown(player_snail, tempSnailMove);
                //std::cout << mot.angle << std::endl;
                //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                //std::cout << mot.lastDirection << std::endl;
                //std::cout << motion.position.x << ", " << motion.position.y << std::endl;
                break;
            case GLFW_KEY_D:
                goRight(player_snail, tempSnailMove);
                //std::cout << mot.angle << std::endl;
                //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                //std::cout << mot.lastDirection << std::endl;
                //std::cout << motion.position.x << ", " << motion.position.y << std::endl;
                break;
            case GLFW_KEY_A:
                goLeft(player_snail, tempSnailMove);
                break;
            case GLFW_KEY_SPACE:
                fallDown(player_snail, tempSnailMove);
                break;
            case GLFW_KEY_ESCAPE:
                running = false;
                ControlsOverlay::removeControlsOverlay();
                notify(Event(Event::PAUSE));
                break;
            }
            if (tempSnailMove == -1) {
                Projectile::Preview::removeCurrent();
                WorldSystem::snailMoves--;
            }

        }

        //        if (snail_move < tempSnailMove && action == GLFW_PRESS)
        //        {
        //            auto& registry = ECS::registry<Motion>;
        //            for (unsigned int i=0; i< registry.components.size(); i++)
        //            {
        //                auto entity = registry.entities[i];
        //                if (ECS::registry<Spider>.has(entity))
        //                {
        //                    float scale = TileSystem::getScale();
        //
        //                    vec2& snail_position = ECS::registry<Destination>.has(player_snail) ?
        //                            ECS::registry<Destination>.get(player_snail).position :
        //                            ECS::registry<Motion>.get(player_snail).position;
        //
        //                    int xCoord = static_cast<int>(snail_position.x / scale);
        //                    int yCoord = static_cast<int>(snail_position.y / scale);
        //
        //                    auto& motEntity = ECS::registry<Motion>.get(entity);
        //                    int xCoordEntity = static_cast<int>(motEntity.position.x / scale);
        //                    int yCoordEntity = static_cast<int>(motEntity.position.y / scale);
        //
        //                    int ai_move=0;
        //                    int tmp_move = ai_move;
        //                    if (xCoordEntity-xCoord==0 && yCoordEntity-yCoord==0) {
        //                    // dont move
        //                    }else {
        //                        if (xCoordEntity-xCoord>=0) {
        //                            goLeft(entity, ai_move);
        //                            if(ai_move == tmp_move){
        //                                if(yCoordEntity-yCoord>0) {
        //                                    goUp(entity, ai_move);
        //                                } else {
        //                                    goDown(entity, ai_move);
        //                                }
        //                            }
        //                        }
        //                        if(ai_move == tmp_move) {
        //                            goRight(entity, ai_move);
        //                            if(ai_move == tmp_move){
        //                                if(yCoordEntity-yCoord>0) {
        //                                    goUp(entity, ai_move);
        //                                } else {
        //                                    goDown(entity, ai_move);
        //                                }
        //                            }
        //                        }
        //                    }
        //                }
        //            }
        //            // presumably the snail moved therefore, remove any projectile preview
        //        }
    }

    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        restart(level);
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
        float scale = TileSystem::getScale();
        ECS::registry<Camera>.components[0].offset.x += scale;

        // Remove any projectile previews if turn ended
        Projectile::Preview::removeCurrent();

        // Can be more than 1 tile per turn. Will probably gain a different amount
        // depending on snail's status
        WorldSystem::snailMoves = 1;
        AISystem::aiMoves = 1;

    }

    // toggle controls overlay
    if (action == GLFW_PRESS && key == GLFW_KEY_C)
        ControlsOverlay::toggleControlsOverlay();

    // Debugging
    // CHANGE: Switched debug key to V so it would not trigger when moving to the right
    if (action == GLFW_PRESS && key == GLFW_KEY_V)
        DebugSystem::in_debug_mode = !DebugSystem::in_debug_mode;
    if (action == GLFW_PRESS && key == GLFW_KEY_P)
        DebugSystem::in_path_debug_mode = !DebugSystem::in_path_debug_mode;

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
    // Remove current previews if mouse moves
    Projectile::Preview::removeCurrent();
}

void WorldSystem::on_mouse_button(int button, int action, int /*mods*/)
{
    // remove prompt on mouse click
    if (action == GLFW_PRESS)
        ControlsOverlay::removeControlsPrompt();

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Check if we would go off screen and die, and prevent that from happening
        vec2& cameraOffset = ECS::registry<Camera>.components[0].offset;
        std::cout << cameraOffset.x << ", " << cameraOffset.y;
        vec2 possibleOffset = { cameraOffset.x + TileSystem::getScale(), cameraOffset.y };
        // Have to get window size this way because no access to window_size_in_game_units
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        if (!offScreen(ECS::registry<Motion>.get(player_snail).position, vec2(w, h), possibleOffset))
        {
            cameraOffset = possibleOffset;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        double mouse_pos_x;
        double mouse_pos_y;
        glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
        vec2 mouse_pos = vec2(mouse_pos_x, mouse_pos_y);
        if (action == GLFW_RELEASE)
        {
            shootProjectile(mouse_pos);
            left_mouse_pressed = false;
            Projectile::Preview::removeCurrent();

        }
        else if (action == GLFW_PRESS)
        {
            left_mouse_pressed = true;
            can_show_projectile_preview_time = std::chrono::high_resolution_clock::now()
                + std::chrono::milliseconds{ PROJECTILE_PREVIEW_DELAY_MS };
        }
    }
}

void WorldSystem::shootProjectile(vec2 mousePos, bool preview /* = false */)
{

    //first we get the position of the mouse_pos relative to the start of the level.
    vec2& cameraOffset = ECS::registry<Camera>.components[0].offset;
    mousePos = mousePos + cameraOffset;

    // now you want to go in the direction of the (mouse_pos - snail_pos), but make it a unit vector
    vec2 snailPosition = ECS::registry<Motion>.get(player_snail).position;
    vec2 projectileVelocity = (mousePos - snailPosition);
    float length = glm::length(projectileVelocity);
    projectileVelocity.x = (projectileVelocity.x / length) * 100;
    projectileVelocity.y = (projectileVelocity.y / length) * 100;
    if (projectileVelocity != vec2(0, 0))
    {
        Projectile::createProjectile(snailPosition, projectileVelocity, preview);
    }

    //shooting a projectile takes your turn.
    if (!preview)
        WorldSystem::snailMoves--;
}

void  WorldSystem::setGLFWCallbacks()
{
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
}
int WorldSystem::snailMoves = 0;
