#include "level_select.hpp"
#include "text.hpp"
#include "spider.hpp"

LevelSelect::LevelSelect(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void LevelSelect::setActive()
{
	Menu::setActive();
	loadEntities();
}

void LevelSelect::step(vec2 window_size_in_game_units)
{

}

void LevelSelect::loadEntities()
{
	// a big bad
	ECS::Entity spider = Spider::createSpider({ 1100, 400 });
	Motion& motion = ECS::registry<Motion>.get(spider);
	motion.scale *= 8;
	ECS::registry<LevelSelectTag>.emplace(spider);
}

void LevelSelect::removeEntities()
{
	for (ECS::Entity entity : ECS::registry<LevelSelectTag>.entities)
	{
		ECS::ContainerInterface::remove_all_components_of(entity);
	}
}

void LevelSelect::exit()
{
	removeEntities();
	notify(Event(Event::MENU_CLOSE, Event::MenuType::LEVEL_SELECT));
}

// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void LevelSelect::on_key(int key, int, int action, int /*mod*/)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			// exit level select (go back to start menu)
			exit();
			break;
		case GLFW_KEY_1:
			notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::DEMO));
			notify(Event(Event::EventType::MENU_CLOSE_ALL));
			break;
		case GLFW_KEY_2:
			notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::DEMO_2));
			notify(Event(Event::EventType::MENU_CLOSE_ALL));
			break;
		}
	}
}

void LevelSelect::on_mouse_button(int button, int action, int /*mods*/)
{
	(void)button;
	(void)action;
}

void LevelSelect::on_mouse_move(vec2 mouse_pos)
{
	(void)mouse_pos;
}
