// internal
#include "menus.hpp"
#include "menus/start_menu.hpp"
#include "menus/level_select.hpp"
#include "menus/pause_menu.hpp"
#include "tiles/tiles.hpp"
#include "text.hpp"
#include "collectible_menu.hpp"

// stlib
#include <sstream>

MenuSystem::MenuSystem(GLFWwindow& window) : window(window) {}

void MenuSystem::start()
{
	ECS::registry<ScreenState>.components[0].darken_screen_factor = 0.f;

	// Update window title
	std::stringstream title_ss;
	title_ss << "A Snail's Pace";
	glfwSetWindowTitle(&window, title_ss.str().c_str());

	assert(menus.empty());
	// reset scale, grid, and camera (may have been changed due to previous level load)
	TileSystem::resetGrid();
	TileSystem::setScale(100.f);
	// clear everything visible
	while (ECS::registry<Motion>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());
	while (ECS::registry<Text>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Text>.entities.back());
	Camera::reset();
	notify(Event(Event::CLOSE_BG));
	openMenu(Event::START_MENU);
}

bool MenuSystem::hasOpenMenu()
{
	return !menus.empty();
}

void MenuSystem::step(vec2 window_size_in_game_units)
{
	// clear delete queue to avoid memory leaks
	//	can't do this in onNotify due to modifying list while iterating over it
	while (!toDelete.empty())
	{
		Menu* menu = toDelete.top();
		menu->setInactive();
		toDelete.pop();
		delete menu;
	}

	// do nothing if no menus are open
	if (!hasOpenMenu())
		return;

	menus.top()->step(window_size_in_game_units);
}

void MenuSystem::onNotify(Event event)
{
	switch (event.type)
	{
	case Event::MENU_CLOSE:
		closeMenu();
		switch (event.menu)
		{
		case Event::MenuType::START_MENU:
			notify(Event(Event::LOAD_LEVEL, 0));
			break;
		case Event::MenuType::PAUSE_MENU:
			notify(Event(Event::UNPAUSE));
			break;
		default:
			break;
		}
		break;
	case Event::MENU_OPEN:
		openMenu(event.menu);
		break;
	case Event::PAUSE:
		// should be no menus open already when pausing
		assert(menus.empty());
		openMenu(Event::MenuType::PAUSE_MENU);
		break;
	case Event::MENU_START:
		// move all current menus to delete queue
		while (!menus.empty())
		{
			Menu* menu = menus.top();
			menus.pop();
			toDelete.push(menu);
		}

		// return to start menu
		start();
		break;
	case Event::LOAD_LEVEL:
		// move all current menus to delete queue
		while (!menus.empty())
		{
			Menu* menu = menus.top();
			menus.pop();
			toDelete.push(menu);
		}

		// notify world to load level by passing event on
		notify(event);
		break;
	case Event::EQUIP_COLLECTIBLE:
	    // just pass it on to world
	    notify(event);
	    break;
    case Event::CLEAR_COLLECTIBLES:
        notify(event);
        break;
    }
}

void MenuSystem::closeMenu()
{
	Menu* oldMenu = menus.top();
	toDelete.push(oldMenu);
	menus.pop();

	if (menus.empty())
	{
		// all menus exited, game should be running
		//	(notify event depends on start or pause menu exit or level selected)
	}
	else
	{	
		// reload next top menu
		Menu* nextMenu = menus.top();
		nextMenu->setActive();
	}
}

void MenuSystem::openMenu(Event::MenuType menu)
{
	Menu* newMenu;

	switch (menu)
	{
	case Event::MenuType::START_MENU:
		newMenu = new StartMenu(window);
		break;
	case Event::MenuType::LEVEL_SELECT:
		newMenu = new LevelSelect(window);
		break;
	case Event::MenuType::PAUSE_MENU:
		newMenu = new PauseMenu(window);
		break;
	case Event::MenuType::COLLECTIBLES_MENU:
        newMenu = new CollectMenu(window);
        break;
	default:
		// no menu opened
		return;
	}

	menus.push(newMenu);
	newMenu->addObserver(this);
}
