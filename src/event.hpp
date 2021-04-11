#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Event
{
public:
	enum EventType { COLLISION, PROJECTILE_POPPED, LOAD_LEVEL, LOAD_SAVE, LOAD_BG, CLOSE_BG, MENU_START, MENU_OPEN, MENU_CLOSE, MENU_CLOSE_ALL, PAUSE, UNPAUSE, TILE_OCCUPIED, TILE_UNOCCUPIED, LEVEL_LOADED, START_DIALOGUE, NEXT_DIALOGUE, RESUME_DIALOGUE, END_DIALOGUE, LEVEL_COMPLETE, NEXT_LEVEL, GAME_OVER, SPLASH };
	// used to determine what menu type is being opened or closed
	enum MenuType { INVALID_MENU, START_MENU, LEVEL_SELECT, PAUSE_MENU, COLLECTIBLES_MENU, LEVEL_COMPLETE_MENU, END_SCREEN};

	EventType type;
	ECS::Entity entity;
	ECS::Entity other_entity;
	int number = -1;
	MenuType menu = INVALID_MENU;
	std::string dialogue;
	int offset;

	// Stats
	int attempts = -1;
	int enemies_killed = -1;
	int projectiles_fired = -1;

	// typed event with no additional arguments
	Event(EventType t) : type(t) {}
	// level load
	Event(EventType t, int numInt) : type(t), number(numInt) {}
	// menu open or close
	Event(EventType t, MenuType m) : type(t), menu(m) {}
    //splash
    Event(EventType t, ECS::Entity& e) : type(t), entity(e) {}
	// collision
	Event(EventType t, ECS::Entity& e, ECS::Entity& o) : type(t), entity(e), other_entity(o) {}
	// start dialogue
    Event(EventType t, std::string d) : type(t), dialogue(d) {}
	Event(EventType t, std::string d, int o) : type(t), dialogue(d), offset(o) {}
	//game end stats
	Event(EventType t, int a, int e, int p): type(t), attempts(a), enemies_killed(e), projectiles_fired(p) {}
};
