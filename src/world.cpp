// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "spider.hpp"
#include "slug.hpp"
#include "projectile.hpp"
#include "ai.hpp"
#include "bird.hpp"
#include "fish.hpp"
#include "tiles/wall.hpp"
#include "tiles/water.hpp"
#include "tiles/vine.hpp"
#include "pebbles.hpp"
#include "render.hpp"
#include "render_components.hpp"
#include "tiles/tiles.hpp"
#include "level_loader.hpp"
#include "load_save.hpp"
#include "controls_overlay.hpp"
#include "parallax_background.hpp"
#include "dialogue.hpp"
#include "npc.hpp"
#include "observer.hpp"
#include "subject.hpp"
#include "collectible.hpp"
#include "particle.hpp"
#include "load_save.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
// Game configuration
const size_t PROJECTILE_PREVIEW_DELAY_MS = 100; // frequency of projectile previews

static std::vector<std::pair<int, std::string> > tutorial_messages = 
{ {200, "Hello, and welcome to A Snail's Pace! This is a turn-based game. Your objective is to make it to the end of each level without dying. Use WASD to move. Each move consumes one turn. If you die, you will respawn at the beginning of the level. Press any key to continue."},
  {200, "In front of you is a spider. Left click to fire a projectile before it reaches you. Spiders kill you on contact. Hold the left mouse button to preview the trajectory. Shooting also consumes your turn."},
  {200, "You will drown in water. Climb the vines instead. Once at the top, stick upside-down to the wall by pressing W."},
  {300, "By now, you've seen the camera move every few turns. You will die if you fall behind. The window title at the top displays when the camera will next move." },
  {300, "To fall back down to the ground, press SPACE." },
  {200, "A few tiles ahead, there is a slug. Like spiders, slugs will kill you on contact. However, they also fire projectiles. Dodge them or destroy them with your own!"},
  {200, "This is an NPC. To interact with them, press E. To advance the interaction, press any key. To stop interacting, press Q." },
  {200, "Above you is a collectible. You can't reach this one, but stay on the lookout so you can look extra fly!"},
  {200, "Nice job. You're almost at the end of the tutorial! If you ever forget the controls, you can press C to display them."} };

static std::vector< std::vector< bool > > first_run;
static int msg_index = 0;

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
    running(false),
    deaths(0),
    enemies_killed(0), 
    projectiles_fired(0),
    left_mouse_pressed(false),
    release_projectile(false),
    level(0),
    snail_move(1), // this might be something we want to load in
    turns_per_camera_move(1),
    projectile_turn_over_time(0.f)
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

	init_audio();
    Mix_PlayMusic(menu_music, -1);
	std::cout << "Loaded music\n";
}

WorldSystem::~WorldSystem() {
    // Destroy music components
    if (menu_music != nullptr)
        Mix_FreeMusic(menu_music);
    if (background_music != nullptr)
        Mix_FreeMusic(background_music);
    if (level_complete_sound != nullptr)
        Mix_FreeChunk(level_complete_sound);
    if (snail_dead_sound != nullptr)
        Mix_FreeChunk(snail_dead_sound);
    if (enemy_dead_sound != nullptr)
        Mix_FreeChunk(enemy_dead_sound);
    if (enemy_nope_sound != nullptr)
        Mix_FreeChunk(enemy_nope_sound);
    if (superspider_spawn_sound != nullptr)
        Mix_FreeChunk(superspider_spawn_sound);
    if (snail_fall_sound != nullptr)
        Mix_FreeChunk(snail_fall_sound);
    if (snail_move_sound != nullptr)
        Mix_FreeChunk(snail_move_sound);
    if (splash_sound != nullptr)
        Mix_FreeChunk(splash_sound);
    if (projectile_fire_sound != nullptr)
        Mix_FreeChunk(projectile_fire_sound);
    if (projectile_break_sound != nullptr)
        Mix_FreeChunk(projectile_break_sound);
    if (dialogue_sound != nullptr)
        Mix_FreeChunk(dialogue_sound);
    if (collectible_sound != nullptr)
        Mix_FreeChunk(collectible_sound);
    Mix_CloseAudio();

    SDL_Quit();

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

    menu_music = Mix_LoadMUS(audio_path("adobeshop.mid").c_str());
    background_music = Mix_LoadMUS(audio_path("Arcade - Battle Network.mid").c_str());
    level_complete_sound = Mix_LoadWAV(audio_path("Victory.wav").c_str());
    snail_dead_sound = Mix_LoadWAV(audio_path("417486__mentoslat__8-bit-death-sound.wav").c_str());
    enemy_dead_sound = Mix_LoadWAV(audio_path("523216__gemesil__death-scream.wav").c_str());
    enemy_nope_sound = Mix_LoadWAV(audio_path("439043__javapimp__lexie-nope.wav").c_str());
    superspider_spawn_sound = Mix_LoadWAV(audio_path("341240__sharesynth__powerup03.wav").c_str());
    snail_move_sound = Mix_LoadWAV(audio_path("240776__f4ngy__card-flip.wav").c_str());
    snail_fall_sound = Mix_LoadWAV(audio_path("350906__cabled-mess__jump-c-04.wav").c_str());
    splash_sound = Mix_LoadWAV(audio_path("110393__soundscalpel-com__water-splash.wav").c_str());
    projectile_fire_sound = Mix_LoadWAV(audio_path("323741__reitanna__mouth-pop.wav").c_str());
    projectile_break_sound = Mix_LoadWAV(audio_path("443328__effectator__quick-clack.wav").c_str());
    dialogue_sound = Mix_LoadWAV(audio_path("431891__syberic__aha.wav").c_str());
    collectible_sound = Mix_LoadWAV(audio_path("428663__jomse__pickupbook4.wav").c_str());

    if (menu_music == nullptr || background_music == nullptr || level_complete_sound == nullptr || snail_dead_sound == nullptr || enemy_dead_sound == nullptr || enemy_nope_sound == nullptr || superspider_spawn_sound == nullptr || snail_move_sound == nullptr || snail_fall_sound == nullptr || splash_sound == nullptr || projectile_fire_sound == nullptr || projectile_break_sound == nullptr || dialogue_sound == nullptr || collectible_sound == nullptr)
        throw std::runtime_error("Failed to load sounds; make sure the data directory is present");

    Volume::set(LoadSaveSystem::getSavedVolume());
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
        die();
    }

    float scale = TileSystem::getScale();
    int xCoord = static_cast<int>(snailMotion.position.x / scale);
    int yCoord = static_cast<int>(snailMotion.position.y / scale);
    ivec2 endCoordinates = TileSystem::getEndCoordinates();


    if (ECS::registry<Turn>.components[0].type == PLAYER_WAITING
            && xCoord == endCoordinates.x && yCoord == endCoordinates.y) {
        running = false;
        ControlsOverlay::removeControlsOverlay();
        Mix_PlayChannel(-1, level_complete_sound, 0);
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

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
	if (left_mouse_pressed && (std::chrono::high_resolution_clock::now() >= can_show_projectile_preview_time))
    {
        // this makes it depend on the preview time length, which isn't great, but at this point we've seen worse lol
        release_projectile = false;

        if (turnType == PLAYER_WAITING)
        {
            double mouse_pos_x, mouse_pos_y;
            glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
            vec2 mouse_pos = vec2(mouse_pos_x, mouse_pos_y);
            shootProjectile(mouse_pos, true);
            can_show_projectile_preview_time = std::chrono::high_resolution_clock::now()
                                               + std::chrono::milliseconds{ PROJECTILE_PREVIEW_DELAY_MS };
        }
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
                for (auto& waterEntity : ECS::registry<WaterTile>.entities)
                {
                    if (WaterTile::splashEntityID != waterEntity.id) {
                        ECS::ContainerInterface::remove_all_components_of(waterEntity);
                        ECS::registry<WaterTile>.remove(waterEntity);
                    }
                }
                WaterTile::splashEntityID = 0;
                ECS::registry<DeathTimer>.remove(entity);
                restart(level);
                return;
            }
        } else if (ECS::registry<Particle>.has(entity) || ECS::registry<Spider>.has(entity)){
            auto& motion = ECS::registry<Motion>.get(entity);
            bool isParticle = ECS::registry<Particle>.has(entity);
            bool isWeatherParticle = ECS::registry<WeatherParticle>.has(entity) || ECS::registry<WeatherParentParticle>.has(entity);
            if(isParticle || isWeatherParticle) {
                motion.scale *= (isWeatherParticle) ? (1-(step_seconds/8)) : (1+(step_seconds/3));
                motion.angle *= (isWeatherParticle) ? (1+(step_seconds)) : 1;
            }
            auto& counter = ECS::registry<DeathTimer>.get(entity);
            counter.counter_ms -= elapsed_ms;
            if (counter.counter_ms < 0)
            {
                if(!isWeatherParticle) {
                    ECS::ContainerInterface::remove_all_components_of(entity);
                }
            }
        }
	}

	if (turnType == PLAYER_WAITING)
    {
        auto tiles = TileSystem::getTiles();
        if (yCoord < tiles.size()) {
            Tile& t = tiles[yCoord][xCoord];
            if (t.type == MESSAGE && first_run[yCoord][xCoord]) {
                auto offset = tutorial_messages[msg_index].first;
                auto message = tutorial_messages[msg_index].second;
                notify(Event(Event::START_DIALOGUE, message, offset));
                msg_index++;
                first_run[yCoord][xCoord] = false;
            }
        }
	    if (snail_move <= 0)
        {
            for (auto& entity : ECS::registry<Fish>.entities) {
                auto& move = ECS::registry<Fish::Move>.get(entity);
                move.hasMoved = false;
            }
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
            for (auto entity : ECS::registry<Projectile>.entities)
            {
                bool isSnailProjectile = ECS::registry<SnailProjectile>.has(entity);
                auto& proj = ECS::registry<Projectile>.get(entity);
                auto& motion = ECS::registry<Motion>.get(entity);
                int maxMoves = isSnailProjectile ? Projectile::snailProjectileMaxMoves : Projectile::aiProjectileMaxMoves;
                float scaleFactor = (1.f - (proj.moved*1.0/maxMoves*1.0));
                motion.scale = proj.origScale * vec2(scaleFactor, scaleFactor);
                proj.moved++;
            }
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
        int move = 1;
        for (auto& entity : ECS::registry<Fish>.entities) {
            fishMove(entity, move);
        }

	    // this works out so that the projectiles move a set amount of time per enemy turn
	    if (ECS::registry<Projectile>.size() != 0)
        {
	        projectile_turn_over_time -= elapsed_ms;
        }

        
        // projectile done moving or no projectiles AND AI path calculated or AI all dead
        // changed from AI.size to Enemy.size
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
                saveGame();
            }
        }

    }
	else if (turnType == CAMERA)
    {
	    if (!ECS::registry<Destination>.has(cameraEntity))
        {
	        turnType = PLAYER_WAITING;
	        saveGame();
        }
    }
}

// Reset the world state to its initial state
void WorldSystem::restart(int newLevel, bool fromSave)
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

	// Load level from data/levels
    level = newLevel;
    LevelLoader lvlldr;
    BackgroundSystem bg(window_size_in_game_units);
    lvlldr.addObserver(&bg);
    lvlldr.loadLevel(newLevel, false, {0,0}, fromSave);

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

    // Reset Turn
    ECS::registry<Turn>.components[0].type = PLAYER_WAITING;

    // for the first attempt of any level, prompt controls overlay
    if (deaths == 0)
        ControlsOverlay::addControlsPrompt();

    AISystem::aiMoved = false;


    if (fromSave)
    {
        json save = LoadSaveSystem::loadLevelFileToJson();
        setFromJson(save);
    }
    else
    {
        Camera::reset();
        turn_number = 0;
    }
    turns_per_camera_move = TileSystem::getTurnsForCameraUpdate();

    snail_move = 1;

    setGLFWCallbacks();

    // save upon restart
    saveGame();
}

void WorldSystem::onNotify(Event event) {

    if (event.type == Event::UNPAUSE)
    {
        setGLFWCallbacks();
        Mix_ResumeMusic();
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
            if (ECS::registry<Enemy>.has(event.other_entity) || ECS::registry<WaterTile>.has(event.other_entity)
                || ECS::registry<SlugProjectile>.has(event.other_entity))
            {
                // Initiate death unless already dying
                if (!ECS::registry<DeathTimer>.has(event.entity))
                {
                    die();
                }
            }
            else if (ECS::registry<Collectible>.has(event.other_entity))
            {
                int const id = ECS::registry<Collectible>.get(event.other_entity).id;
                ECS::registry<Inventory>.components[0].collectibles.insert(id);
                // Equip collectible (creates new entity)
                Collectible::equip(event.entity, id);
                Mix_PlayChannel(-1, collectible_sound, 0);
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
                // Checking Projectile - Enemy / Enemy Projectile collisions
                if (ECS::registry<Invincible>.has(event.other_entity))
                {
                    Mix_PlayChannel(-1, enemy_nope_sound, 0);
                    // remove the projectile
                    ECS::ContainerInterface::remove_all_components_of(event.entity);
                }
                else if (ECS::registry<Enemy>.has(event.other_entity))
                {
                    Mix_PlayChannel(-1, enemy_dead_sound, 0);

                    // tile no longer occupied by enemy
                    float scale = TileSystem::getScale();
                    auto& motion = ECS::registry<Motion>.get(event.other_entity);
                    int xCoord = static_cast<int>(motion.position.x / scale);
                    int yCoord = static_cast<int>(motion.position.y / scale);
                    Tile& t = TileSystem::getTiles()[yCoord][xCoord];
                    t.removeOccupyingEntity();
                    bool wasSpider = ECS::registry<Spider>.has(event.other_entity);
                    enemies_killed++;
                    ECS::Entity explodingSpider;
                    if (wasSpider) {
                        Spider::createExplodingSpider(motion, explodingSpider);
                    }
                    // Remove the enemy but not the projectile
                    ECS::ContainerInterface::remove_all_components_of(event.other_entity);
                }
                else if (ECS::registry<SlugProjectile>.has(event.other_entity))
                {
                    Mix_PlayChannel(-1, projectile_break_sound, 0);
                    // remove the enemy projectile
                    ECS::ContainerInterface::remove_all_components_of(event.other_entity);
                }
            }
        }
        //spider to spider collision creates super spider
        if (ECS::registry<Spider>.has(event.entity)) {
            if (ECS::registry<Spider>.has(event.other_entity)) {
                std::cout << "2 spiders in the same tile" << std::endl;
                Mix_PlayChannel(-1, superspider_spawn_sound, 0);
                float scale = TileSystem::getScale();
                auto& motion1 = ECS::registry<Motion>.get(event.entity);
                int xCoord = static_cast<int>(motion1.position.x / scale);
                int yCoord = static_cast<int>(motion1.position.y / scale);
                Tile& t = TileSystem::getTiles()[yCoord][xCoord];
                // maybe I need 2 calls to remove both of them?
                //t.removeOccupyingEntity();
                ECS::Entity superSpider;
                vec2 pos = { t.x, t.y };
                t.removeOccupyingEntity();
                t.removeOccupyingEntity();
                ECS::ContainerInterface::remove_all_components_of(event.entity);
                ECS::ContainerInterface::remove_all_components_of(event.other_entity);
                SuperSpider::createSuperSpider(pos, superSpider);
                t.addOccupyingEntity();
            }
        }
    }
    else if (event.type == Event::LOAD_SAVE)
    {
        running = true;
        DebugSystem::in_debug_mode = false;
        ControlsOverlay::toggleControlsOverlayOff();

        int saved_level = LoadSaveSystem::getSavedLevelNum();
        // save file should exist and level should exist
        assert(saved_level != -1 && saved_level < levels.size());
        level = saved_level;
        Mix_PlayMusic(background_music, -1);
        restart(level, true);
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

        // Play background music
        Mix_PlayMusic(background_music, -1);

        restart(level);

        // if selecting tutorial from level menu reset messages
        if (level == 0) {
            msg_index = 0;
            auto tiles = TileSystem::getTiles();
            first_run = std::vector< std::vector< bool > >(tiles.size(), std::vector<bool>(tiles[0].size(), true));
        }
        
    } else if (event.type == Event::SPLASH) {
        if((event.entity.id != WaterTile::splashEntityID) && (ECS::registry<WaterTile>.has(event.entity))) {
            Mix_PlayChannel(-1, splash_sound, 0);
            WaterTile::onNotify(Event::SPLASH, event.entity);
        }
    }
    else if (event.type == Event::MENU_START)
    {
        Mix_PlayMusic(menu_music, -1);
    }
}

void WorldSystem::setFromJson(nlohmann::json const& saved)
{
    level = saved[WorldKeys::LEVEL_NUM_KEY];
    turn_number = saved[WorldKeys::TURNS_KEY];
    deaths = saved[WorldKeys::NUM_DEATHS_KEY];
    enemies_killed = saved[WorldKeys::NUM_ENEMIES_KILLS_KEY];
    projectiles_fired = saved[WorldKeys::NUM_PROJECTILES_FIRED_KEY];
    Camera::reset({ saved[WorldKeys::CAMERA_KEY]["x"], saved[WorldKeys::CAMERA_KEY]["y"] });
    if (level == 0)
    {
        msg_index = saved[WorldKeys::MSG_INDEX_KEY];

        for (json row : saved[WorldKeys::FIRST_RUN_VEC_KEY])
        {
            std::vector<bool> vec;
            for (bool tile : row)
            {
                vec.push_back(tile);
            }
            first_run.push_back(vec);
        }
    }
}

void WorldSystem::writeToJson(nlohmann::json& toSave)
{
    toSave[WorldKeys::LEVEL_NUM_KEY] = level;
    toSave[WorldKeys::TURNS_KEY] = turn_number;
    toSave[WorldKeys::NUM_DEATHS_KEY] = deaths;
    toSave[WorldKeys::NUM_ENEMIES_KILLS_KEY] = enemies_killed;
    toSave[WorldKeys::NUM_PROJECTILES_FIRED_KEY]= projectiles_fired;

    vec2 cameraPos = Camera::getPosition();
    toSave[WorldKeys::CAMERA_KEY]["x"] = cameraPos.x;
    toSave[WorldKeys::CAMERA_KEY]["y"] = cameraPos.y;

    // save tutorial status
    if (level == 0)
    {
        toSave[WorldKeys::MSG_INDEX_KEY] = msg_index;

        for (auto& vec : first_run)
        {
            json row;
            for (bool tile : vec)
            {
                row.push_back(tile);
            }
            toSave[WorldKeys::FIRST_RUN_VEC_KEY].push_back(row);
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

bool WorldSystem::offScreenExceptNegativeYWithBuffer(vec2 const& pos, vec2 window_size_in_game_units, vec2 cameraOffset, int buffer)
{
    vec2 offsetPos = { pos.x - cameraOffset.x, pos.y - cameraOffset.y };
    if(offsetPos.x > window_size_in_game_units.x) {
        // haha
    }
    return (offsetPos.x + buffer < 0.f || offsetPos.x - buffer > window_size_in_game_units.x || offsetPos.y > window_size_in_game_units.y);
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

//only entities that can round the corner should be calling this function
void WorldSystem::changeDirection(Motion& motion, Tile& currTile, Tile& nextTile, int defaultDirection, ECS::Entity& entity) 
{
    Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
    dest.position = { nextTile.x, nextTile.y };
    // give velocity to reach destination in set time
    // this velocity will be set to 0 once destination is reached in physics.cpp
    motion.velocity = (dest.position - motion.position)/k_move_seconds;

    bool areRoundingCorner = motion.velocity.x != 0 && motion.velocity.y != 0;

    if (defaultDirection == DIRECTION_SOUTH || defaultDirection == DIRECTION_NORTH)
    {
        if (!areRoundingCorner)
        {
            doY(motion, currTile, nextTile);
            rotate(currTile, motion, nextTile);
            doX(motion, currTile, nextTile, defaultDirection);
        }
        else 
        {
            //just face the right way.
            //compare the lastDirection (currentDirection) to the directionInput. If they don't match, flip it first.
            auto& direction = ECS::registry<DirectionInput>.get(entity).direction;
            if (motion.lastDirection != direction) 
            {
                //then it's facing the wrong way so flip it.
                motion.scale.x *= -1;
            }
        }
    }
    else
    {
        if (!areRoundingCorner)
        {
            doX(motion, currTile, nextTile, defaultDirection);
            rotate(currTile, motion, nextTile);
            doY(motion, currTile, nextTile);
        }
        else 
        {
            //just face the right way.
            //compare the lastDirection (currentDirection) to the directionInput. If they don't match, flip it first.
            auto& direction = ECS::registry<DirectionInput>.get(entity).direction;
            if (motion.lastDirection != direction)
            {
                //then it's facing the wrong way so flip it.
                motion.scale.x *= -1;
            }
        }
    }
}

void WorldSystem::goLeft(ECS::Entity &entity, int &moves) 
{
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
        if (nextTile.type == INACCESSIBLE) return;
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
    auto& camera = ECS::registry<Camera>.entities[0];
    vec2 cameraOffset = ECS::registry<Motion>.get(camera).position;
    int cameraOffsetX = cameraOffset.x / TileSystem::getScale();
    int window_size = window_size_in_game_units.x;
    int cameraRight = (window_size / scale) + cameraOffsetX;

    if (xCoord + 1 > tiles[yCoord].size() - 1 || xCoord + 1 >= cameraRight) {
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
        if (nextTile.type == INACCESSIBLE) return;
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
        if (nextTile.type == INACCESSIBLE) return;
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
        if (nextTile.type == INACCESSIBLE) return;
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

void WorldSystem::fishMove(ECS::Entity& entity, int& moves) {
    float scale = TileSystem::getScale();
    auto& tiles = TileSystem::getTiles();

    //if fish is going up

    auto& motion = ECS::registry<Motion>.get(entity);
    int xCoord = static_cast<int>(motion.position.x / scale);
    int yCoord = static_cast<int>(motion.position.y / scale);
    auto& move = ECS::registry<Fish::Move>.get(entity);
    if (move.hasMoved == false) {
        if (move.direction == true) {
            if (yCoord - 1 < 0) {
                return;
            }
            Tile upDest = tiles[yCoord - 1][xCoord];
            if (upDest.type == WALL) {
                move.direction = false;
                move.hasMoved = true;
                return;
            }
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { upDest.x, upDest.y };
            motion.velocity = (dest.position - motion.position) / k_move_seconds;
            moves--;
            move.hasMoved = true;
            return;
        }
        else {
            if (yCoord + 1 == tiles.size()) {
                move.direction = true;
                return;
            }
            Tile downDest = tiles[yCoord + 1][xCoord];
            if (downDest.type == WATER) {
                move.direction = true;
            }
            Destination& dest = ECS::registry<Destination>.has(entity) ? ECS::registry<Destination>.get(entity) : ECS::registry<Destination>.emplace(entity);
            dest.position = { downDest.x, downDest.y };
            motion.velocity = (dest.position - motion.position) / k_move_seconds;
            moves--;
            move.hasMoved = true;
            return;
        }
    }
    else {
        return;
    }
}


// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
    if (ECS::registry<DeathTimer>.has(player_snail)) {
        return;
    }

    if (action == GLFW_PRESS)
    {
        // remove prompt on key press
        ControlsOverlay::removeControlsPrompt();
        
        // tutorial messages
        auto& snailMotion = ECS::registry<Motion>.get(player_snail);
        float scale = TileSystem::getScale();
        int xCoord = static_cast<int>(snailMotion.position.x / scale);
        int yCoord = static_cast<int>(snailMotion.position.y / scale);
        Tile& t = TileSystem::getTiles()[yCoord][xCoord];
        if (t.type == MESSAGE) {
            notify(Event(Event::END_DIALOGUE));
        }

        bool shouldReturn = true;

        switch (key)
        {
        // Pause
        case GLFW_KEY_ESCAPE:
            {
                running = false;
                left_mouse_pressed = false;
                SnailProjectile::Preview::removeCurrent();
                ControlsOverlay::removeControlsOverlay();
                Mix_PauseMusic();
                notify(Event(Event::PAUSE));
                break;
            }
        // toggle controls overlay
        case GLFW_KEY_C:
            ControlsOverlay::toggleControlsOverlay();
            break;
        // Debugging
        case GLFW_KEY_V:
            DebugSystem::in_debug_mode = !DebugSystem::in_debug_mode;
            break;
        // Path debugging
        case GLFW_KEY_P:
            DebugSystem::in_path_debug_mode = !DebugSystem::in_path_debug_mode;
            break;
        default:
            {
                shouldReturn = false;
                break;
            }
        }

        if (shouldReturn) return;
    }

    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R)
    {
        restart(level);
        return;
    }
    
    // Snail action if alive and has turns remaining.
	if (!ECS::registry<DeathTimer>.has(player_snail) && action == GLFW_PRESS)
	{
		if (!left_mouse_pressed && ECS::registry<Turn>.components[0].type == PLAYER_WAITING && (snail_move > 0))
		{
			switch (key)
			{
            // These logs are pretty useful to see what scale, angle and direction the snail should have
			case GLFW_KEY_W:
                    ECS::registry<DirectionInput>.get(player_snail).direction = DIRECTION_NORTH;
                    goUp(player_snail, snail_move);
                    if (snail_move == 0) Mix_PlayChannel(-1, snail_move_sound, 0);
				    break;
			case GLFW_KEY_S:
                    ECS::registry<DirectionInput>.get(player_snail).direction = DIRECTION_SOUTH;
                    goDown(player_snail, snail_move);
                    if (snail_move == 0) Mix_PlayChannel(-1, snail_move_sound, 0);
                    break;
			case GLFW_KEY_D:
                    ECS::registry<DirectionInput>.get(player_snail).direction = DIRECTION_EAST;
                    goRight(player_snail, snail_move);
                    if (snail_move == 0) Mix_PlayChannel(-1, snail_move_sound, 0);
                    break;
			case GLFW_KEY_A:
                    ECS::registry<DirectionInput>.get(player_snail).direction = DIRECTION_WEST;
                    goLeft(player_snail, snail_move);
                    if (snail_move == 0) Mix_PlayChannel(-1, snail_move_sound, 0);
				    break;
            case GLFW_KEY_SPACE:
                    fallDown(player_snail, snail_move);
                    if (snail_move == 0) Mix_PlayChannel(-1, snail_fall_sound, 0);
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
                        Mix_PlayChannel(-1, dialogue_sound, 0);
                        ECS::registry<Turn>.components[0].type = NPC_ENCOUNTER;
                        break; // assuming only one NPC nearby at a time
                    }
                }
                break;
			}
		}
        else if (ECS::registry<Turn>.components[0].type == NPC_ENCOUNTER)
        {
            switch (key)
            {
            // exit dialogue
            case GLFW_KEY_Q:
                stopNPC();
                break;
            // step encounter
            default:
                stepNPC();
                break;
            }
        }
    }
}

void WorldSystem::on_mouse_move(vec2 /*mouse_pos*/)
{
    // do nothing (see setGLFWCallbacks() for more detail)
    return;
}

void WorldSystem::on_mouse_button(int button, int action, int /*mods*/)
{
    if (ECS::registry<DeathTimer>.has(player_snail)) {
        return;
    }

    // remove prompt on mouse click
    if (action == GLFW_PRESS)
    {
        ControlsOverlay::removeControlsPrompt();

        // tutorial messages
        auto& snailMotion = ECS::registry<Motion>.get(player_snail);
        float scale = TileSystem::getScale();
        int xCoord = static_cast<int>(snailMotion.position.x / scale);
        int yCoord = static_cast<int>(snailMotion.position.y / scale);
        Tile& t = TileSystem::getTiles()[yCoord][xCoord];
        if (t.type == MESSAGE) {
            notify(Event(Event::END_DIALOGUE));
        }
    }

    TurnType& turnType = ECS::registry<Turn>.components[0].type;
    if ((turnType == PLAYER_WAITING) && (button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_RELEASE))
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
        if (action == GLFW_RELEASE)
        {
            if ((turnType == PLAYER_WAITING) && release_projectile)
            {
                double mouse_pos_x;
                double mouse_pos_y;
                glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);
                vec2 mouse_pos = vec2(mouse_pos_x, mouse_pos_y);

                projectiles_fired++;
                shootProjectile(mouse_pos);
                SnailProjectile::Preview::removeCurrent();

                if (snail_move == 0) Mix_PlayChannel(-1, projectile_fire_sound, 0);
            }
            left_mouse_pressed = false;
        }
        else if (action == GLFW_PRESS)
        {
            release_projectile = true;
            left_mouse_pressed = true;
            // give double the time for first preview (for slow clickers)
            can_show_projectile_preview_time = std::chrono::high_resolution_clock::now()
                                               + (4*std::chrono::milliseconds{ PROJECTILE_PREVIEW_DELAY_MS });
        }
    }
    else if ((ECS::registry<Turn>.components[0].type == NPC_ENCOUNTER) && (action == GLFW_PRESS))
    {
        stepNPC();
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
    // instead of firing from the centre of the snail, fire from 7/10 a tile in the direction of the mouse
    if (snailPosition == mousePos) {
        return;
    vec2 projectilePosition = snailPosition + 0.7f * normalize(mousePos - snailPosition) * TileSystem::getScale();
	vec2 projectileVelocity = (mousePos - snailPosition);
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

void WorldSystem::setGLFWCallbacks()
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

void WorldSystem::stopNPC()
{
    NPC& npc = ECS::registry<NPC>.get(encountered_npc);
    npc.endEncounter();

    json toSave;
    writeToJson(toSave);
    LoadSaveSystem::writeLevelFile(toSave);

    ECS::registry<Turn>.components[0].type = PLAYER_WAITING;
}

void WorldSystem::stepNPC()
{
    NPC& npc = ECS::registry<NPC>.get(encountered_npc);
    npc.stepEncounter();
    if (!npc.isActive)
    {
        saveGame();

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

        saveGame();
    }
}

void WorldSystem::saveGame()
{
    if (!ECS::registry<DeathTimer>.has(player_snail))
    {
        json toSave;
        writeToJson(toSave);
        LoadSaveSystem::writeLevelFile(toSave);
    }
}

void WorldSystem::die()
{
    // scream, reset timer, increment deaths, delete save file
    ECS::registry<DeathTimer>.emplace(player_snail);
    Mix_PlayChannel(-1, snail_dead_sound, 0);

    deaths++;

    // no redos sorry :(
    LoadSaveSystem::deleteSaveFile();
}

vec2 WorldSystem::window_size_in_game_units;
