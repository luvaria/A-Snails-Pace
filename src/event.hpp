#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Event
{
public:
	enum EventType { COLLISION, LOAD_LEVEL, LOAD_BG, CLOSE_BG, MENU_START, MENU_OPEN, MENU_CLOSE, MENU_CLOSE_ALL, PAUSE,
	        UNPAUSE, TILE_OCCUPIED, TILE_UNOCCUPIED, LEVEL_LOADED, START_DIALOGUE, NEXT_DIALOGUE, RESUME_DIALOGUE, END_DIALOGUE,
	        EQUIP_COLLECTIBLE, CLEAR_COLLECTIBLES };
	// used to determine what menu type is being opened or closed
	enum MenuType { INVALID_MENU, START_MENU, LEVEL_SELECT, PAUSE_MENU, COLLECTIBLES_MENU };

	EventType type;
	ECS::Entity entity;
	ECS::Entity other_entity;
	int number = -1;
	MenuType menu = INVALID_MENU;
	std::string dialogue;

	// typed event with no additional arguments
	Event(EventType t) : type(t) {}
	// level load or player equip
	Event(EventType t, int numInt) : type(t), number(numInt) {}
	// menu open or close
	Event(EventType t, MenuType m) : type(t), menu(m) {}
	// collision
	Event(EventType t, ECS::Entity& e, ECS::Entity& o) : type(t), entity(e), other_entity(o) {}
	// start dialogue
	Event(EventType t, std::string d) : type(t), dialogue(d) {}
};
