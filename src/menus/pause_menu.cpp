#include "pause_menu.hpp"
#include "text.hpp"

// pause menu button size (since you can't tell based on the Text component)
const vec2 buttonScale = { 250.f, 33.3f };

PauseMenu::PauseMenu(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void PauseMenu::setActive()
{
	Menu::setActive();
	loadEntities();
}

void PauseMenu::step(vec2 /*window_size_in_game_units*/)
{
	const auto ABEEZEE_REGULAR = Font::load(ABEEZEE_REGULAR_PATH);
	const auto ABEEZEE_ITALIC = Font::load(ABEEZEE_ITALIC_PATH);

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

void PauseMenu::loadEntities()
{
	ECS::registry<ScreenState>.components[0].darken_screen_factor = 0.2f;

	const auto ABEEZEE_REGULAR = Font::load(ABEEZEE_REGULAR_PATH);

	// paused
	auto titleTextEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		titleTextEntity,
		Text("Game Paused", ABEEZEE_REGULAR, { 445.f, 360.f })
	);
	Text& titleText = ECS::registry<Text>.get(titleTextEntity);
	titleText.scale = SUBTITLE_SCALE;
	titleText.colour = TITLE_COLOUR;
	ECS::registry<PauseMenuTag>.emplace(titleTextEntity);

	// quit game button
	auto quitGameEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		quitGameEntity,
		Text("Quit", ABEEZEE_REGULAR, { 560.f, 420.f })
	);
	Text& quitText = ECS::registry<Text>.get(quitGameEntity);
	quitText.colour = DEFAULT_COLOUR;
	quitText.scale = OPTION_SCALE;
	ECS::registry<MenuButton>.emplace(quitGameEntity, ButtonEventType::QUIT_GAME);
	ECS::registry<PauseMenuTag>.emplace(quitGameEntity);
	buttonEntities.push_back(quitGameEntity);

	ECS::Entity panelEntity = ECS::Entity();
	std::string panelKey = "pause_panel";
	ShadedMesh& panelResource = cache_resource(panelKey);
	if (panelResource.effect.program.resource == 0)
	{
		panelResource = ShadedMesh();
		RenderSystem::createSprite(panelResource, textures_path("pause_panel.png"), "textured", false);
	}

	ECS::registry<ShadedMeshRef>.emplace(panelEntity, panelResource, RenderBucket::OVERLAY_2);

	Motion& minVolMotion = ECS::registry<Motion>.emplace(panelEntity);
	minVolMotion.angle = 0.f;
	minVolMotion.velocity = { 0, 0 };
	minVolMotion.position = { 600, 375 };
	minVolMotion.scale = static_cast<vec2>(panelResource.texture.size) / static_cast<vec2>(panelResource.texture.size).x * 500.f;

	ECS::registry<Overlay>.emplace(panelEntity);
	ECS::registry<PauseMenuTag>.emplace(panelEntity);
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
	ECS::registry<ScreenState>.components[0].darken_screen_factor = 0.f;
	removeEntities();
	notify(Event(Event::MENU_CLOSE, Event::MenuType::PAUSE_MENU));
}

void PauseMenu::on_key(int key, int, int action, int /*mod*/)
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

void PauseMenu::on_mouse_button(int button, int action, int /*mods*/)
{
	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		selectedKeyEvent();
	}
}

void PauseMenu::on_mouse_move(vec2 mouse_pos)
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

void PauseMenu::selectedKeyEvent()
{
	for (MenuButton& b : ECS::registry<MenuButton>.components)
	{
		// perform action for button being hovered over (and pressed)
		if (b.selected)
		{
			switch (b.event)
			{
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
