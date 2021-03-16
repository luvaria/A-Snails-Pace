#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Event
{
public:
	enum EventType { COLLISION, LOAD_LEVEL, LOAD_BG, CLOSE_BG, MENU_START, MENU_OPEN, MENU_CLOSE, MENU_CLOSE_ALL, PAUSE, UNPAUSE, LEVEL_COMPLETE, NEXT_LEVEL, GAME_OVER, STATS, TILE_OCCUPIED, TILE_UNOCCUPIED};
	// used to determine what menu type is being opened or closed
	enum MenuType { INVALID_MENU, START_MENU, LEVEL_SELECT, PAUSE_MENU, LEVEL_COMPLETE_MENU, END_SCREEN};

	EventType type;
	ECS::Entity entity;
	ECS::Entity other_entity;
	int level = -1;
	MenuType menu = INVALID_MENU;

	//stats
	int attempts;
	int enemies_killed;
	int projectiles_fired;

	// pause unpause, backgrounds, return to start menu
	Event(EventType t) : type(t) {}
	// level load
	Event(EventType t, int l) : type(t), level(l) {}
	// menu open or close
	Event(EventType t, MenuType m) : type(t), menu(m) {}
	// collision
	Event(EventType t, ECS::Entity& e, ECS::Entity& o) : type(t), entity(e), other_entity(o) {}
	//game end stats
	Event(EventType t, int a, int e, int p): type(t), attempts(a), enemies_killed(e), projectiles_fired(p) {}
};
