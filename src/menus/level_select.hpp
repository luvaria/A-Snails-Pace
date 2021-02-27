#include "menu.hpp"

class LevelSelect : public Menu
{
public:
	LevelSelect(GLFWwindow& window);
	void setActive();
	void step(vec2 window_size_in_game_units);

private:
	void loadEntities();
	void removeEntities();
	void exit();

	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);
	void on_mouse_button(int button, int action, int /*mods*/);
};

// component tag
struct LevelSelectTag {};
