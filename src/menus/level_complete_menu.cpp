#include "level_complete_menu.hpp"
#include "text.hpp"
#include <world.hpp>

// pause menu button size (since you can't tell based on the Text component)
const vec2 buttonScale = { 250.f, 33.3f };

LevelCompleteMenu::LevelCompleteMenu(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void LevelCompleteMenu::setActive()
{
	Menu::setActive();
	loadEntities();
}

void LevelCompleteMenu::step(vec2 /*window_size_in_game_units*/)
{
	const auto ABEEZEE_REGULAR = Font::load("data/fonts/abeezee/ABeeZee-Regular.otf");
	const auto ABEEZEE_ITALIC = Font::load("data/fonts/abeezee/ABeeZee-Italic.otf");

	// update button colour based on mouseover
	auto& buttonContainer = ECS::registry<MenuButton>;
	auto& textContainer = ECS::registry<Text>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];
		ECS::Entity buttonEntity = buttonContainer.entities[i];
		Text& buttonText = textContainer.get(buttonEntity);

		if (button.selected)
		{
			buttonText.colour = HIGHLIGHT_COLOUR;
			buttonText.font = ABEEZEE_ITALIC;
		}
		else
		{
			buttonText.colour = DEFAULT_COLOUR;
			buttonText.font = ABEEZEE_REGULAR;
		}
	}
}

void LevelCompleteMenu::loadEntities()
{
	ECS::registry<ScreenState>.components[0].darken_screen_factor = 0.2f;

	const auto ABEEZEE_REGULAR = Font::load("data/fonts/abeezee/ABeeZee-Regular.otf");

	// paused
	auto titleTextEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		titleTextEntity,
		Text("Level Complete!", ABEEZEE_REGULAR, { 450.f, 250.f })
	);
	Text& titleText = ECS::registry<Text>.get(titleTextEntity);
	titleText.scale = SUBTITLE_SCALE;
	titleText.colour = TITLE_COLOUR;
	ECS::registry<LevelCompleteMenuTag>.emplace(titleTextEntity);

	//next level button
	auto nextLevelEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		nextLevelEntity,
		Text("Next Level", ABEEZEE_REGULAR, { 550.f, 325.0f })
	);
	Text& nextLevelText = ECS::registry<Text>.get(nextLevelEntity);
	nextLevelText.colour = DEFAULT_COLOUR;
	nextLevelText.scale = OPTION_SCALE;
	ECS::registry<MenuButton>.emplace(nextLevelEntity, ButtonEventType::NEXT_LEVEL);
	ECS::registry<LevelCompleteMenuTag>.emplace(nextLevelEntity);
	buttonEntities.push_back(nextLevelEntity);

	// quit game button
	auto quitGameEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		quitGameEntity,
		Text("Quit", ABEEZEE_REGULAR, { 550.f, 375.0f })
	);
	Text& quitText = ECS::registry<Text>.get(quitGameEntity);
	quitText.colour = DEFAULT_COLOUR;
	quitText.scale = OPTION_SCALE;
	ECS::registry<MenuButton>.emplace(quitGameEntity, ButtonEventType::QUIT_GAME);
	ECS::registry<LevelCompleteMenuTag>.emplace(quitGameEntity);
	buttonEntities.push_back(quitGameEntity);
}

void LevelCompleteMenu::removeEntities()
{
	for (ECS::Entity entity : ECS::registry<LevelCompleteMenuTag>.entities)
	{
		ECS::ContainerInterface::remove_all_components_of(entity);
	}
}

void LevelCompleteMenu::exit()
{
	ECS::registry<ScreenState>.components[0].darken_screen_factor = 0.f;
	removeEntities();
	notify(Event(Event::MENU_CLOSE, Event::MenuType::PAUSE_MENU));
}

void LevelCompleteMenu::on_key(int key, int, int action, int /*mod*/)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			// exit menu and unpause game
			exit();
			break;
		case GLFW_KEY_DOWN:
			selectNextButton();
			break;
		case GLFW_KEY_UP:
			selectPreviousButton();
			break;
		case GLFW_KEY_ENTER:
			selectedKeyEvent();
			break;
		case GLFW_KEY_Q:
			notify(Event(Event::MENU_START));
			break;
		}
	}
}

void LevelCompleteMenu::on_mouse_button(int button, int action, int /*mods*/)
{
	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		selectedKeyEvent();
	}
}

void LevelCompleteMenu::on_mouse_move(vec2 mouse_pos)
{
	auto& buttonContainer = ECS::registry<MenuButton>;
	auto& textContainer = ECS::registry<Text>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];
		ECS::Entity buttonEntity = buttonContainer.entities[i];
		Text& buttonText = textContainer.get(buttonEntity);

		// check whether button is being hovered over
		button.selected = mouseover(vec2(buttonText.position.x, buttonText.position.y), buttonScale, mouse_pos);
	}
}

void LevelCompleteMenu::selectedKeyEvent()
{
	for (MenuButton& b : ECS::registry<MenuButton>.components)
	{
		// perform action for button being hovered over (and pressed)
		if (b.selected)
		{
			switch (b.event)
			{
			case ButtonEventType::NEXT_LEVEL:
				notify(Event(Event::NEXT_LEVEL));
				break;
			case ButtonEventType::QUIT_GAME:
				// exit game and return to start menu
				notify(Event(Event::MENU_START));
				break;
			default:
				break;
			}
		}
	}
}
