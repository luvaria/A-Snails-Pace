# pragma once

#include "menus/menu.hpp"
#include "observer.hpp"
#include "subject.hpp"

// stlib
#include <vector>
#include <stack>

class MenuSystem : public Observer, public Subject
{
public:
	MenuSystem(GLFWwindow& window);

	// starts menu system, beginning at start menu
	void start();

	// returns true if at least one menu is open, false otherwise
	bool hasOpenMenu();

	// runs top menu in stack
	//	does nothing if no menus open
	void step(vec2 window_size_in_game_units);

	void onNotify(Event event);

	// closes current top menu
	void closeMenu();

	// open new menu on top
	void openMenu(Event::MenuType menu);
	
private:
	GLFWwindow& window;
	std::stack<Menu*> menus;
	// delete queue (upon menu close notify)
	std::stack<Menu*> toDelete;
};
