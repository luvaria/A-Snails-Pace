#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Event
{
public:
	enum EventType { COLLISION, LOAD_LEVEL, MENU_START, MENU_OPEN, MENU_CLOSE, MENU_CLOSE_ALL, PAUSE, UNPAUSE };
	// used to determine what level to load
	enum Level { INVALID_LEVEL, DEMO, DEMO_2 };
	// used to determine what menu type is being opened or closed
	enum MenuType { INVALID_MENU, START_MENU, LEVEL_SELECT, PAUSE_MENU };

	EventType type;
	ECS::Entity entity;
	ECS::Entity other_entity;
	Level level = INVALID_LEVEL;
	MenuType menu = INVALID_MENU;

	// pause unpause, return to start menu
	Event(EventType t) : type(t) {}
	// level load
	Event(EventType t, Level l) : type(t), level(l) {}
	// menu open or close
	Event(EventType t, MenuType m) : type(t), menu(m) {}
	// collision
	Event(EventType t, ECS::Entity& e, ECS::Entity& o) : type(t), entity(e), other_entity(o) {}
};
