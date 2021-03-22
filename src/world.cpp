// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "projectile.hpp"
#include "fish.hpp"
#include "ai.hpp"
#include "bird.hpp"
#include "tiles/wall.hpp"
#include "tiles/water.hpp"
#include "tiles/vine.hpp"
#include "pebbles.hpp"
#include "render.hpp"
#include "render_components.hpp"
#include "tiles/tiles.hpp"
#include "level_loader.hpp"
#include "controls_overlay.hpp"
#include "parallax_background.hpp"
#include "dialogue.hpp"
#include "npc.hpp"
#include "observer.hpp"
#include "subject.hpp"
#include "collectible.hpp"
#include "particle.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
// Game configuration
const size_t PROJECTILE_PREVIEW_DELAY_MS = 100; // frequency of projectile previews

int level = 0;

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
    running(false),
    deaths(0),
    enemies_killed(0), 
    projectiles_fired(0),
    left_mouse_pressed(false),
    snail_move(1), // this might be something we want to load in
    turns_per_camera_move(1),
    projectile_turn_over_time(0.f),
    slug_projectile_turn_over_time(0.f)
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

    Camera::reset();
    turns_per_camera_move = TileSystem::getTurnsForCameraUpdate();

    auto turnEntity = ECS::Entity();
    auto& turn = ECS::registry<Turn>.emplace(turnEntity);
    turn.type = PLAYER_WAITING;


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
    
    glfwTerminate();
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

    int points = ECS::registry<Inventory>.components[0].points;
    // Updating window title with moves remaining and deaths
    std::stringstream title_ss;
    title_ss << "Moves taken: " << turn_number;
    title_ss << ", ";
	title_ss << "Camera will move in " << turns_per_camera_move - (turn_number % turns_per_camera_move) << " turn(s)";
    title_ss << ", ";
    title_ss << "Deaths: " << deaths;
    title_ss << ", ";
    title_ss << "Points: " << points;
    glfwSetWindowTitle(window, title_ss.str().c_str());

    auto& cameraEntity = ECS::registry<Camera>.entities[0];
    vec2& cameraOffset = ECS::registry<Motion>.get(cameraEntity).position;

	// Kill snail if off screen
    auto& snailMotion = ECS::registry<Motion>.get(player_snail);
    if (!ECS::registry<DeathTimer>.has(player_snail)
        && offScreen(snailMotion.position, window_size_in_game_units, cameraOffset))
    {
        ECS::registry<DeathTimer>.emplace(player_snail);
        Mix_PlayChannel(-1, salmon_dead_sound, 0);
    }

    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();
    int xCoord = static_cast<int>(snailMotion.position.x / scale);
    int yCoord = static_cast<int>(snailMotion.position.y / scale);
    ivec2 endCoordinates = TileSystem::getEndCoordinates();

    if (ECS::registry<Turn>.components[0].type == PLAYER_WAITING
            && xCoord == endCoordinates.x && yCoord == endCoordinates.y) {
        running = false;
        ControlsOverlay::removeControlsOverlay();
        notify(Event(Event::LEVEL_COMPLETE));
    }

	// remove any offscreen projectiles
	for (auto entity : ECS::registry<Projectile>.entities)
	{
		auto projectilePosition = ECS::registry<Motion>.get(entity).position;
		if (offScreen(projectilePosition, window_size_in_game_units, cameraOffset))
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

    float step_seconds = 1.0f * (elapsed_ms / 1000.f);
    
    // Processing the snail state
    assert(ECS::registry<ScreenState>.components.size() <= 1);
    auto& screen = ECS::registry<ScreenState>.components[0];

    for (auto entity : ECS::registry<DeathTimer>.entities)
    {
        if(ECS::registry<Snail>.has(entity)) {
            auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
            texmesh.texture.color = {0.8, 0.2, 0.2};
            
            // Progress timer
            auto& counter = ECS::registry<DeathTimer>.get(entity);
            counter.counter_ms -= elapsed_ms;

            // Reduce window brightness if any of the present snails is dying
            screen.darken_screen_factor = 1 - counter.counter_ms / 500.f;

            if(WaterTile::splashEntityID!=0) {
                Motion& mot = ECS::registry<Motion>.get(entity);
                auto mesh_ptr = ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
                float vol = mesh_ptr->mesh.original_size.x * mesh_ptr->mesh.original_size.x * ((mesh_ptr->mesh.original_size.x+mesh_ptr->mesh.original_size.y)/4);
                float snailDensity = 0.23;
                float mass = snailDensity * vol;
                float drownedDist = ((vol - mass) * 9.81)*step_seconds;
                mot.position.y += drownedDist;
            }
            // Restart the game once the death timer expired
            if (counter.counter_ms < 0)
            {
                ECS::registry<DeathTimer>.remove(entity);
                for (auto& entity : ECS::registry<WaterTile>.entities)
                {
                    if(WaterTile::splashEntityID!=entity.id) {
                        ECS::ContainerInterface::remove_all_components_of(entity);
                        ECS::registry<WaterTile>.remove(entity);
                    }
                }
                WaterTile::splashEntityID = 0;
                ECS::registry<DeathTimer>.remove(entity);
                deaths++;
                restart(level);
                return;
            }
        } else if (ECS::registry<Particle>.has(entity) || ECS::registry<Spider>.has(entity)){
            auto& motion = ECS::registry<Motion>.get(entity);
            bool isWeatherParticle = ECS::registry<WeatherParticle>.has(entity);
            motion.scale *= (isWeatherParticle) ? (1-(step_seconds/8)) : (1+(step_seconds/3));
            motion.angle *= (isWeatherParticle) ? (1+(step_seconds)) : 1;

            auto& counter = ECS::registry<DeathTimer>.get(entity);
            counter.counter_ms -= elapsed_ms;
            if (counter.counter_ms < 0)
            {
                if(isWeatherParticle && offScreen(motion.position, window_size_in_game_units, cameraOffset)) {
                    ECS::ContainerInterface::remove_all_components_of(entity);
                } else {
                    ECS::ContainerInterface::remove_all_components_of(entity);
                }
            }
        }
	}

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
	if (turnType == PLAYER_WAITING)
    {
	    if (snail_move <= 0)
        {
	        turnType = PLAYER_UPDATE;
        }
    }
	else if (turnType == PLAYER_UPDATE)
    {
	    SnailProjectile::Preview::removeCurrent();
	    if ((snail_move <= 0) && !ECS::registry<Destination>.has(player_snail))
        {
            turnType = ENEMY;
            AISystem::aiMoved = false;
            projectile_turn_over_time = k_projectile_turn_ms;
            snail_move = 1;
            turn_number++;
            if ((turn_number % turns_per_camera_move) == 0)
            {
                Camera::update(k_move_seconds);
            }
        }
    }
	else if (turnType == ENEMY)
    {
	    // this works out so that the projectiles move a set amount of time per enemy turn
	    if (ECS::registry<Projectile>.size() != 0)
        {
	        projectile_turn_over_time -= elapsed_ms;
        }

        
        // projectile done moving or no projectiles AND AI path calculated or AI all dead
        if (((ECS::registry<Projectile>.size() != 0 && projectile_turn_over_time <= 0) 
            || (ECS::registry<Projectile>.size() == 0))
            && (AISystem::aiMoved || (ECS::registry<AI>.size() == 0)))
        {
            // In the following two cases, if true, all the enemies will have moved
            // Camera has to move
            if ((ECS::registry<Destination>.size() == 1) && ECS::registry<Destination>.has(cameraEntity))
            {
                 turnType = CAMERA;
            }

            // Camera does not have to move
            else if (ECS::registry<Destination>.size() == 0)
            {
                turnType = PLAYER_WAITING;
            }
        }
    }
	else if (turnType == CAMERA)
    {
	    if (!ECS::registry<Destination>.has(cameraEntity))
	        turnType = PLAYER_WAITING;
    }
}

// Reset the world state to its initial state
void WorldSystem::restart(int newLevel)
{
    // Debugging for memory/component leaks
    ECS::ContainerInterface::list_all_components();
    std::cout << "Restarting\n";

    // Reset the game speed
    current_speed = 1.f;

    // Reset screen darken factor (manual restart can remove DeathTimer without resetting)
    auto& screen = ECS::registry<ScreenState>.components[0];
    screen.darken_screen_factor = 0;

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

    // Load backgrounds
    notify(Event(Event::LOAD_BG));

	// Load level from data/levels
    level = newLevel;
    LevelLoader::loadLevel(newLevel);
    // register NPCs in observer pattern
    notify(Event(Event::LEVEL_LOADED));
    // can't access player_snail in level loader
    player_snail = ECS::registry<Player>.entities[0];
    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(player_snail).reference_to_cache;
    texmesh.texture.color = {1, 1, 1};
    if (ECS::registry<Inventory>.components[0].equipped != -1)
    {
        Collectible::equip(player_snail, ECS::registry<Inventory>.components[0].equipped);
    }
    texmesh.texture.color = {1, 1, 1};
    
    // Reset Camera
    Camera::reset();
    turns_per_camera_move = TileSystem::getTurnsForCameraUpdate();

    // Reset Turn
    ECS::registry<Turn>.components[0].type = PLAYER_WAITING;

    // for the first level, prompt controls overlay
    if (level == 0 && deaths == 0)
        ControlsOverlay::addControlsPrompt();

	snail_move = 1;
	turn_number = 0;

    AISystem::aiMoved = false;

    setGLFWCallbacks();
}

void WorldSystem::onNotify(Event event) {

    if (event.type == Event::UNPAUSE)
    {
        setGLFWCallbacks();
        ControlsOverlay::addControlsOverlayIfOn();
        notify(Event(Event::RESUME_DIALOGUE));
        running = true;
    }

    if (event.type == Event::NEXT_LEVEL) {
        setGLFWCallbacks();
        ControlsOverlay::addControlsOverlayIfOn();
        running = true;
        //load next level
        level += 1;
        if (level < levels.size()) {
            restart(level);
        }
        else {
            //game end screen
            running = false;
            ControlsOverlay::removeControlsOverlay();
            notify(Event(Event::GAME_OVER, deaths, enemies_killed, projectiles_fired));
        }
    }
    
    else if (event.type == Event::COLLISION) {

        // Collisions involving snail
        if (ECS::registry<Snail>.has(event.entity))
        {
            // Check collisions that result in death
            if (ECS::registry<Spider>.has(event.other_entity) || ECS::registry<WaterTile>.has(event.other_entity)
                || ECS::registry<Slug>.has(event.other_entity) || ECS::registry<SlugProjectile>.has(event.other_entity))
            {
                // Initiate death unless already dying
                if (!ECS::registry<DeathTimer>.has(event.entity))
                {
                    // Scream, reset timer, and make the snail sink
                    ECS::registry<DeathTimer>.emplace(event.entity);
                    Mix_PlayChannel(-1, salmon_dead_sound, 0);
                }
            }
            else if (ECS::registry<Collectible>.has(event.other_entity))
            {
                int const id = ECS::registry<Collectible>.get(event.other_entity).id;
                ECS::registry<Inventory>.components[0].collectibles.insert(id);
                // Equip collectible (creates new entity)
                Collectible::equip(event.entity, id);
                // Remove the collectible from the map
                ECS::ContainerInterface::remove_all_components_of(event.other_entity);
            }
        }

        // Collisions involving the snail projectiles
        if (ECS::registry<SnailProjectile>.has(event.entity))
        {
            // Don't collide with a preview projectile (ie. all enemies should fall under here)
            if (!ECS::registry<SnailProjectile::Preview>.has(event.entity))
            {
                // Checking Projectile - Spider collisions
                if (ECS::registry<Spider>.has(event.other_entity) || ECS::registry<Slug>.has(event.other_entity) 
                    || ECS::registry<SlugProjectile>.has(event.other_entity))
                {
                    // tile no longer occupied by spider
                    float scale = TileSystem::getScale();
                    auto& motion = ECS::registry<Motion>.get(event.other_entity);
                    int xCoord = static_cast<int>(motion.position.x / scale);
                    int yCoord = static_cast<int>(motion.position.y / scale);
                    Tile& t = TileSystem::getTiles()[yCoord][xCoord];
                    t.removeOccupyingEntity();

                    enemies_killed++;
                    ECS::Entity expoldingSpider;
                    if (ECS::registry<Spider>.has(event.other_entity)) {
                        Spider::createExplodingSpider(motion, expoldingSpider);
                    }
                    // Remove the spider but not the projectile
                    ECS::ContainerInterface::remove_all_components_of(event.other_entity);
                }
            }
        }
    }
    else if (event.type == Event::LOAD_LEVEL)
    {
        running = true;
        DebugSystem::in_debug_mode = false;
        deaths = 0;
        projectiles_fired = 0;
        enemies_killed = 0;

        ControlsOverlay::toggleControlsOverlayOff();

        // level index exists
        assert(event.number >= 0 && event.number < levels.size());
        level = event.number;

        restart(level);
    } else if (event.type == Event::SPLASH) {
        if((event.entity.id != WaterTile::splashEntityID) && (ECS::registry<WaterTile>.has(event.entity))) {
            WaterTile::onNotify(Event::SPLASH, event.entity);
        }
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
//                motion.scale.y = -motion.scale.y;
            }
            break;
        default:
            if (motion.lastDirection != defaultDirection) {
//                motion.scale.x = -motion.scale.x;
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
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_NORTH;
        }
    }
    else {
        if (motion.lastDirection != DIRECTION_SOUTH) {
            motion.scale.x = -motion.scale.x;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
}

void WorldSystem::rotate(Tile& currTile, Motion& motion, Tile& nextTile) {
    if (abs(currTile.x - nextTile.x) > 0 && abs(currTile.y - nextTile.y) > 0) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        if (abs(motion.angle) == PI / 2) {
            motion.angle = motion.lastDirection == DIRECTION_NORTH ? 0 : abs(2 * motion.angle);
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
    motion.velocity = (dest.position - motion.position)/k_move_seconds;
}

void WorldSystem::goLeft(ECS::Entity &entity, int &moves) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    auto& camera = ECS::registry<Camera>.entities[0];
    vec2 cameraOffset = ECS::registry<Motion>.get(camera).position;
    int cameraOffsetX = cameraOffset.x / TileSystem::getScale();
    if (xCoord - 1 < 0 || xCoord - 1 < cameraOffsetX) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile leftTile = tiles[yCoord][xCoord - 1];
    if (leftTile.type == INACCESSIBLE) return;
    Tile nextTile = currTile;
    if (abs(motion.angle) != PI / 2 && (leftTile.type == WALL)) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            if(motion.lastDirection != DIRECTION_WEST)
                motion.scale.x = -motion.scale.x;
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
            motion.angle = motion.angle == 0 ? PI : 0;
            motion.scale.x = -1*motion.scale.x;
            yCord = (motion.angle == -PI / 2 ? yCoord + 1 : yCoord - 1);
            if ((yCord < 0 && yCord > tiles.size() - 1) || (xCoord - 1 < 0)) {
                motion.angle = motion.angle == 0 ? PI : 0;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
            nextTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][xCoord - 1];
            Tile sideTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][(xCoord)];
            if (!(nextTile.type == WALL || leftTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
                motion.angle = motion.angle == 0 ? PI : 0;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
        }
        nextTile = nextTile.type == WALL || leftTile.type == VINE ? leftTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_WEST, entity);
    }
    else if (abs(motion.angle) == PI / 2 && leftTile.type == VINE && currTile.type == VINE) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x;
        motion.lastDirection = DIRECTION_WEST;
        motion.angle = 0;
    } else if (motion.angle == -PI/2 && tiles[motion.lastDirection == DIRECTION_NORTH ? (yCoord - 1) : (yCoord + 1)][xCoord - 1].type == WALL) {
//        motion.scale = { motion.scale.y, motion.scale.x };
//        motion.scale.x = motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : motion.scale.x;
        motion.angle = motion.lastDirection == DIRECTION_NORTH ? PI : 0;
        motion.lastDirection = DIRECTION_WEST;
    }
    else if (motion.angle == PI/2 && tiles[motion.lastDirection == DIRECTION_NORTH ? (yCoord + 1) : (yCoord - 1)][xCoord - 1].type == WALL) {
//        motion.scale = { motion.scale.y, motion.scale.x };
//        motion.scale.x = motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : motion.scale.x;
        motion.angle = motion.lastDirection == DIRECTION_SOUTH ? PI : 0;
        motion.lastDirection = DIRECTION_WEST;
    }else if (abs(motion.angle) == PI / 2 && currTile.type == VINE) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x;
        motion.lastDirection = DIRECTION_WEST;
        motion.angle = 0;
    }
    
    if(currTile.x != nextTile.x || currTile.y != nextTile.y) {
        moves--;
    }
}

void WorldSystem::goRight(ECS::Entity& entity, int& moves) {
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
    if (rightTile.type == INACCESSIBLE) return;
    Tile nextTile = currTile;
    if (abs(motion.angle) != PI / 2 && rightTile.type == WALL) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
//            motion.scale = { motion.scale.y, motion.scale.x };
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
            motion.angle = motion.angle == 0 ? PI : 0;
            motion.scale.x = -1*motion.scale.x;
            yCord = abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1);
            if ((yCord < 0 && yCord > tiles.size() - 1) || (xCoord + 1 > tiles[yCord].size() - 1)) {
                motion.angle = motion.angle == 0 ? PI : 0;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
            nextTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][xCoord + 1];
            Tile sideTile = tiles[abs(motion.angle) == PI ? (yCoord - 1) : (yCoord + 1)][(xCoord)];
            if (!(nextTile.type == WALL || rightTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
                motion.angle = motion.angle == 0 ? PI : 0;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
        }
        nextTile = nextTile.type == WALL || rightTile.type == VINE ? rightTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_EAST, entity);
    }
    else if (abs(motion.angle) == PI / 2 && rightTile.type == VINE && currTile.type == VINE) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x;
        motion.lastDirection = DIRECTION_EAST;
        motion.angle = 0;
    } else if (motion.angle == -PI/2 && tiles[motion.lastDirection == DIRECTION_NORTH ? (yCoord + 1) : (yCoord - 1)][xCoord + 1].type == WALL) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : motion.scale.x;
        motion.angle = motion.lastDirection == DIRECTION_NORTH ? 0 : PI;
        motion.lastDirection = DIRECTION_EAST;
    }
    else if (motion.angle == PI/2 && tiles[motion.lastDirection == DIRECTION_NORTH ? (yCoord - 1) : (yCoord + 1)][xCoord + 1].type == WALL) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : motion.scale.x;
        motion.angle = motion.lastDirection == DIRECTION_NORTH ? PI : 0;
        motion.lastDirection = DIRECTION_EAST;
    } else if (abs(motion.angle) == PI / 2 && currTile.type == VINE) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.scale.x = motion.angle == -PI / 2 ? motion.lastDirection == DIRECTION_NORTH ? motion.scale.x : -motion.scale.x
            : motion.lastDirection == DIRECTION_NORTH ? -motion.scale.x : motion.scale.x;
        motion.lastDirection = DIRECTION_EAST;
        motion.angle = 0;
    }
    
    if(currTile.x != nextTile.x || currTile.y != nextTile.y) {
        moves--;
    }
}

void WorldSystem::goUp(ECS::Entity& entity, int& moves) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    if (yCoord - 1 < 0) {
        return;
    }
    Tile currTile = tiles[yCoord][xCoord];
    Tile upTile = tiles[yCoord-1][xCoord];
    if (upTile.type == INACCESSIBLE) return;
    Tile nextTile = currTile;
    if (currTile.type == VINE && abs(motion.angle) == 0) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if(abs(currTile.x - nextTile.x) == 0 && abs(currTile.y - nextTile.y) == 0) {
//            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI / 2 : -PI / 2;
            motion.lastDirection = DIRECTION_NORTH;
        }
    } else if (currTile.type == VINE && upTile.type != WALL && abs(motion.angle) == PI) {
//        motion.scale = { motion.scale.y, motion.scale.x };
        motion.angle = motion.lastDirection == DIRECTION_EAST ? PI/2 : -PI/2;
        motion.lastDirection = DIRECTION_NORTH;
    } else if (abs(motion.angle) == PI/2 && upTile.type == WALL) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
//            motion.scale = { motion.scale.y, motion.scale.x };
            if(motion.lastDirection == DIRECTION_NORTH) {
                motion.lastDirection = motion.angle == PI / 2 ? DIRECTION_EAST : DIRECTION_WEST;
            } else {
                motion.lastDirection = motion.angle == PI / 2 ? DIRECTION_WEST : DIRECTION_EAST;

            }
            motion.angle = PI;
        }
    }
    else if (abs(motion.angle) == PI / 2) {

        int xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
        if (xCord > tiles[(yCoord - 1)].size() - 1) {
            return;
        }
        nextTile = tiles[(yCoord-1)][xCord];
        Tile sideTile = tiles[(yCoord)][xCord];
        if(!(nextTile.type == WALL || upTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
            motion.angle = -1*motion.angle;
            motion.scale.x = -1*motion.scale.x;
            xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
            if (xCord > tiles[(yCoord - 1)].size() - 1) {
                motion.angle = -1*motion.angle;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
            nextTile = tiles[(yCoord-1)][xCord];
            Tile sideTile = tiles[(yCoord)][xCord];
            if (!(nextTile.type == WALL || upTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
                motion.angle = -1*motion.angle;
                motion.scale.x = -1*motion.scale.x;
                return;
            }
        }
        nextTile = nextTile.type == WALL || upTile.type == VINE ? upTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_NORTH, entity);
    }

    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        moves--;
    }
}

void WorldSystem::goDown(ECS::Entity& entity, int& moves) {
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
    Tile downTile = tiles[yCoord + 1][xCoord];
    if (downTile.type == INACCESSIBLE) return;
    Tile nextTile = currTile;
    if (currTile.type == VINE && abs(motion.angle) == PI) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
//            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? PI / 2 : -PI / 2;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    } else if (downTile.type == WALL) {
        nextTile = tiles[yCoord][xCoord];
        if (motion.angle != 0 && abs(currTile.x - nextTile.x) == 0 && abs(currTile.x - nextTile.x) == 0) {
            changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH, entity);
//            motion.scale = { motion.scale.y, motion.scale.x };
            if(motion.angle == -PI / 2) {
                motion.lastDirection = motion.lastDirection == DIRECTION_NORTH ? DIRECTION_EAST : DIRECTION_WEST;
            } else if(motion.angle == PI / 2) {
                motion.lastDirection = motion.lastDirection == DIRECTION_SOUTH ? DIRECTION_EAST : DIRECTION_WEST;
            }
            
//            if(motion.angle == PI)
//            {
//                motion.scale.x = -motion.scale.x;
//            }
//            motion.lastDirection = motion.angle == -PI / 2 && motion.lastDirection == DIRECTION_NORTH ? DIRECTION_EAST : DIRECTION_WEST;
            motion.angle = 0;
        }
    }
    else if (currTile.type == VINE && abs(motion.angle) == 0) {
        nextTile = tiles[yCoord][xCoord];
        changeDirection(motion, currTile, nextTile, motion.lastDirection, entity);
        if (abs(currTile.x - nextTile.x) == 0 && abs(currTile.y - nextTile.y) == 0) {
//            motion.scale = { motion.scale.y, motion.scale.x };
            motion.angle = motion.lastDirection == DIRECTION_WEST ? -PI / 2 : PI / 2;
            motion.lastDirection = DIRECTION_SOUTH;
        }
    }
    else if (abs(motion.angle) == PI / 2) {

        int xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
        if (xCord > tiles[(yCoord + 1)].size() - 1) {
            return;
        }
        nextTile = tiles[(yCoord + 1)][xCord];
         Tile sideTile = tiles[(yCoord)][xCord];
         if (!(nextTile.type == WALL || downTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
             motion.angle = -1*motion.angle;
             motion.scale.x = -1*motion.scale.x;
             xCord = (motion.angle == -PI / 2 ? xCoord + 1 : xCoord - 1);
             if (xCord > tiles[(yCoord + 1)].size() - 1) {
                 motion.angle = -1*motion.angle;
                 motion.scale.x = -1*motion.scale.x;
                 return;
             }
             nextTile = tiles[(yCoord + 1)][xCord];
             Tile sideTile = tiles[(yCoord)][xCord];
             if (!(nextTile.type == WALL || downTile.type == VINE) && (sideTile.type == EMPTY || sideTile.type == VINE)) {
                 motion.angle = -1*motion.angle;
                 motion.scale.x = -1*motion.scale.x;
                 return;
             }
        }
        nextTile = nextTile.type == WALL || downTile.type == VINE ? downTile : nextTile;
        changeDirection(motion, currTile, nextTile, DIRECTION_SOUTH, entity);
    }
    
    if (currTile.x != nextTile.x || currTile.y != nextTile.y) {
        moves--;
    }
}

void WorldSystem::fallDown(ECS::Entity& entity, int& moves) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);

    Tile upTile = tiles[yCoord + 1][xCoord];
    if (upTile.type == WALL) {
        return;
    }

    int tempMove = moves; // so we don't decrement moves multiple times for one fall
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
            motion.velocity = (dest.position - motion.position)/k_move_seconds;
            tempMove--;
        }
        else if (t.type == WALL) {
            //std::cout << "here" << std::endl;
            //std::cout << xCoord << ", " << i << std::endl;
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { tiles[i - 1][xCoord].x, tiles[i - 1][xCoord].y };
            // give velocity to reach destination in set time
            // this velocity will be set to 0 once destination is reached in physics.cpp
            motion.velocity = (dest.position - motion.position)/k_move_seconds;
            tempMove--;

            if (abs(motion.angle) == PI / 2) {
                goDown(entity, tempMove);
            }
            else if (motion.angle == PI) {
                if (motion.lastDirection == DIRECTION_WEST) {
                    if(tiles[i-1][xCoord].type != VINE)
                        motion.scale.x = -motion.scale.x;
                    goDown(entity, tempMove);
                    motion.lastDirection = tiles[i-1][xCoord].type == VINE ? DIRECTION_SOUTH : DIRECTION_WEST;
                }
                else {
                    if(tiles[i-1][xCoord].type != VINE)
                        motion.scale.x = -motion.scale.x;
                    goDown(entity, tempMove);
                    motion.lastDirection = tiles[i-1][xCoord].type == VINE ? DIRECTION_SOUTH : DIRECTION_EAST;
                }
            }
            i = tiles.size();
        }
        else if (t.type == INACCESSIBLE)
        {
            return;
        }
    }
    if (tempMove < moves)
    {
        moves--;
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
		if ((ECS::registry<Turn>.components[0].type == PLAYER_WAITING) && (action == GLFW_PRESS))
		{
			switch (key)
			{
            // These logs are pretty useful to see what scale, angle and direction the snail should have
			case GLFW_KEY_W:
                    goUp(player_snail, snail_move);
                    //std::cout << mot.angle << std::endl;
                    //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                    //std::cout << mot.lastDirection << std::endl;
				break;
			case GLFW_KEY_S:
                    goDown(player_snail, snail_move);
                    //std::cout << mot.angle << std::endl;
                    //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                    //std::cout << mot.lastDirection << std::endl;
                break;
			case GLFW_KEY_D:
                    goRight(player_snail, snail_move);
                    //std::cout << mot.angle << std::endl;
                    //std::cout << mot.scale.x << ", " << mot.scale.y << std::endl;
                    //std::cout << mot.lastDirection << std::endl;
                break;
			case GLFW_KEY_A:
                    goLeft(player_snail, snail_move);
				break;
            case GLFW_KEY_SPACE:
                    fallDown(player_snail, snail_move);
                break;
            case GLFW_KEY_E:
                Motion& playerMotion = ECS::registry<Motion>.get(player_snail);
                for (ECS::Entity npcEntity : ECS::registry<NPC>.entities)
                {
                    Motion& npcMotion = ECS::registry<Motion>.get(npcEntity);
                    vec2 displacement = playerMotion.position - npcMotion.position;
                    if ((abs(displacement.x) < TileSystem::getScale() * 1.5f) && (abs(displacement.y) < TileSystem::getScale() * 1.5f))
                    {
                        encountered_npc = npcEntity;
                        ECS::registry<NPC>.get(encountered_npc).beginEncounter();
                        ECS::registry<Turn>.components[0].type = NPC_ENCOUNTER;
                        break; // assuming only one NPC nearby at a time
                    }
                }
                break;
			}
		}
        else if ((ECS::registry<Turn>.components[0].type == NPC_ENCOUNTER) && (action == GLFW_PRESS))
        {
            NPC& npc = ECS::registry<NPC>.get(encountered_npc);

            switch (key)
            {
            // exit dialogue
            case GLFW_KEY_Q:
                npc.endEncounter();
                ECS::registry<Turn>.components[0].type = PLAYER_WAITING;
                break;
            // step encounter
            case GLFW_KEY_E:
                npc.stepEncounter();
                if (!npc.isActive)
                {
                    ECS::registry<Turn>.components[0].type = PLAYER_WAITING;
                }
                if (npc.timesTalkedTo >= 2)
                {
                    // after two interactions, npc disappears

                    // update tile type to EMPTY
                    Motion& npcMotion = ECS::registry<Motion>.get(encountered_npc);
                    float scale = TileSystem::getScale();
                    TileSystem::getTiles()[npcMotion.position.y / scale][npcMotion.position.x / scale].type = EMPTY;

                    // remove npc and its hat
                    if (ECS::registry<Equipped>.has(encountered_npc))
                    {
                        ECS::Entity hatEntity = ECS::registry<Equipped>.get(encountered_npc).collectible;
                        ECS::ContainerInterface::remove_all_components_of(hatEntity);
                    }
                    ECS::ContainerInterface::remove_all_components_of(encountered_npc);
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

        restart(level);
    }

    // toggle controls overlay
    if (action == GLFW_PRESS && key == GLFW_KEY_C)
        ControlsOverlay::toggleControlsOverlay();

    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        running = false;
        ControlsOverlay::removeControlsOverlay();
        notify(Event(Event::PAUSE));
    }

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
    SnailProjectile::Preview::removeCurrent();
}

void WorldSystem::on_mouse_button(int button, int action, int /*mods*/)
{
    // remove prompt on mouse click
    if (action == GLFW_PRESS)
        ControlsOverlay::removeControlsPrompt();

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
    if (turnType == PLAYER_WAITING)
    {
        if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_RELEASE))
        {
            // Check if we would go off screen and die, and prevent that from happening
            auto& cameraEntity = ECS::registry<Camera>.entities[0];
            vec2& cameraOffset = ECS::registry<Motion>.get(cameraEntity).position;
            float scale = TileSystem::getScale();
            vec2 possibleOffset = { cameraOffset.x + scale, cameraOffset.y };
            // Have to get window size this way because no access to window_size_in_game_units
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            if (!offScreen(ECS::registry<Motion>.get(player_snail).position, vec2(w, h), possibleOffset))
            {
                Camera::update(k_move_seconds);
                ECS::registry<Turn>.components[0].type = CAMERA;
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
                projectiles_fired++;
                shootProjectile(mouse_pos);
                left_mouse_pressed = false;
                SnailProjectile::Preview::removeCurrent();

            }
            else if (action == GLFW_PRESS)
            {
                left_mouse_pressed = true;
                can_show_projectile_preview_time = std::chrono::high_resolution_clock::now()
                                                   + std::chrono::milliseconds{ PROJECTILE_PREVIEW_DELAY_MS };
            }
        }
    }

}

void WorldSystem::shootProjectile(vec2 mousePos, bool preview /* = false */)
{

	// first we get the position of the mouse_pos relative to the start of the level.
    auto& cameraEntity = ECS::registry<Camera>.entities[0];
    vec2& cameraOffset = ECS::registry<Motion>.get(cameraEntity).position;
	mousePos = mousePos + cameraOffset;

	// now you want to go in the direction of the (mouse_pos - snail_pos), but make it a unit vector
	vec2 snailPosition = ECS::registry<Motion>.get(player_snail).position;
    // instead of firing from the centre of the snail, fire from half a tile in the direction of the mouse
    vec2 projectilePosition = snailPosition + normalize(mousePos - snailPosition) * TileSystem::getScale() / 2.f;
	vec2 projectileVelocity = (mousePos - projectilePosition);
	float length = glm::length(projectileVelocity);
	projectileVelocity.x = (projectileVelocity.x / length) * TileSystem::getScale() * 2;
	projectileVelocity.y = (projectileVelocity.y / length) * TileSystem::getScale() * 2;
	if (projectileVelocity != vec2(0, 0))
	{
        SnailProjectile::createProjectile(projectilePosition, projectileVelocity, preview);
	}
	
	// shooting a projectile takes your turn.
    if (!preview)
    {
        snail_move--;
        projectile_turn_over_time = k_projectile_turn_ms;
    }
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
