#include "menu.hpp"

class StartMenu : public Menu
{
public:
	StartMenu(GLFWwindow& window);
	void setActive();
	void step(vec2 window_size_in_game_units);

private:
	void loadEntities();
	void removeEntities();
	void exit();

	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);
	void on_mouse_button(int button, int action, int /*mods*/);

	void selectedKeyEvent();

	// only for this subclass, may be useful for all subclasses?
	void updateDisabled(MenuButton& button);
};

// component tag
struct StartMenuTag {};
