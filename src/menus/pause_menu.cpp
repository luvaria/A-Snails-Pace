#include "pause_menu.hpp"
#include "text.hpp"

PauseMenu::PauseMenu(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void PauseMenu::setActive()
{
	Menu::setActive();
	loadEntities();
}

void PauseMenu::step(vec2 window_size_in_game_units)
{

}

void PauseMenu::loadEntities()
{
	// paused
	auto titleTextEntity = ECS::Entity();
	auto notoRegular = Font::load("data/fonts/Noto/NotoSans-Regular.ttf");
	ECS::registry<Text>.insert(
		titleTextEntity,
		Text("Game Paused", notoRegular, { 450.f, 200.f })
	);
	Text& titleText = ECS::registry<Text>.get(titleTextEntity);
	titleText.colour = { 0.984f, 0.690f, 0.231f };
	ECS::registry<PauseMenuTag>.emplace(titleTextEntity);
}

void PauseMenu::removeEntities()
{
	for (ECS::Entity entity : ECS::registry<PauseMenuTag>.entities)
	{
		ECS::ContainerInterface::remove_all_components_of(entity);
	}
}

void PauseMenu::exit()
{
	removeEntities();
	notify(Event(Event::MENU_CLOSE, Event::MenuType::PAUSE_MENU));
}

// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void PauseMenu::on_key(int key, int, int action, int mod)
{
	(void)key;
	(void)action;
	(void)mod;

	// exit menu and unpause game
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
	{
		exit();
	}
	// exit game and return to start menu
	else if (action == GLFW_PRESS && key == GLFW_KEY_Q)
	{
		notify(Event(Event::MENU_START));
	}
}

void PauseMenu::on_mouse_button(int button, int action, int /*mods*/)
{
	(void)button;
	(void)action;
}

void PauseMenu::on_mouse_move(vec2 mouse_pos)
{
	(void)mouse_pos;
}
