#include "start_menu.hpp"
#include "text.hpp"
#include "snail.hpp"
#include "load_save.hpp"
#include "../tiles/tiles.hpp"
#include "../tiles/wall.hpp"
#include "../tiles/vine.hpp"

// start menu button size (since you can't tell based on the Text component)
const vec2 buttonScale = { 250.f, 33.3f };

StartMenu::StartMenu(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void StartMenu::setActive()
{
	TileSystem::setScale(100.f);
	Menu::setActive();
	loadEntities();
}

void StartMenu::step(vec2 /*window_size_in_game_units*/)
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

		updateDisabled(button);

        buttonText.alpha = button.disabled ? 0.5f : 1.f;

		if (button.selected && !button.disabled)
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

void StartMenu::loadEntities()
{
	// a big boi
	ECS::Entity snail = Snail::createSnail({ 350, 460 });
	Motion& motion = ECS::registry<Motion>.get(snail);
	motion.scale *= 6;
	ECS::registry<StartMenuTag>.emplace(snail);

	// decorative tiles
	// assuming 1200 x 800
	float scale = TileSystem::getScale();
	// bottom platform
	for (int x = 0; x < 12; x++)
	{
		ECS::Entity wall = WallTile::createWallTile({ (x + 0.5f) * scale, 7.5f * scale });
		ECS::registry<StartMenuTag>.emplace(wall);
	}
	// vines
	for (int x = 0; x < 12; x++)
	{
		if (x == 0 || x == 1 || x == 10 || x == 11)
		{
			for (int y = 1; y < 7; y++)
			{
				ECS::Entity vine = VineTile::createVineTile({ (x + 0.5f) * scale, (y + 0.5f) * scale });
				ECS::registry<StartMenuTag>.emplace(vine);
			}
		}
	}

	const auto ABEEZEE_REGULAR = Font::load(ABEEZEE_REGULAR_PATH);
	const auto VIGA_REGULAR = Font::load(VIGA_REGULAR_PATH);

	// title text
	// probably best to replace with a png/mesh
	auto titleTextEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		titleTextEntity,
		Text("A Snail's Pace", VIGA_REGULAR, { 550.0f, 300.0f })
	);
	Text& titleText = ECS::registry<Text>.get(titleTextEntity);
	titleText.colour = TITLE_COLOUR;
	ECS::registry<StartMenuTag>.emplace(titleTextEntity);

	// start game button
	auto startGameEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		startGameEntity,
		Text("Start", ABEEZEE_REGULAR, { 650.0f, 350.0f })
	);
	Text& startText = ECS::registry<Text>.get(startGameEntity);
	startText.colour = DEFAULT_COLOUR;
	startText.scale = OPTION_SCALE;
	ECS::registry<MenuButton>.emplace(startGameEntity, ButtonEventType::START_GAME);
	ECS::registry<StartMenuTag>.emplace(startGameEntity);
	buttonEntities.push_back(startGameEntity);

	// level select button
	auto selectLevelEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		selectLevelEntity,
		Text("Select level", ABEEZEE_REGULAR, { 650.0f, 400.0f })
	);
	Text& selectText = ECS::registry<Text>.get(selectLevelEntity);
	selectText.colour = DEFAULT_COLOUR;
	selectText.scale *= OPTION_SCALE;
	ECS::registry<MenuButton>.emplace(selectLevelEntity, ButtonEventType::SELECT_LEVEL);
	ECS::registry<StartMenuTag>.emplace(selectLevelEntity);
	buttonEntities.push_back(selectLevelEntity);

    // collectibles menu button
    auto collectiblesEntity = ECS::Entity();
    ECS::registry<Text>.insert(
            collectiblesEntity,
            Text("Collectibles", ABEEZEE_REGULAR, { 650.0f, 450.0f })
    );
    Text& collectiblesText = ECS::registry<Text>.get(collectiblesEntity);
    collectiblesText.colour = DEFAULT_COLOUR;
    collectiblesText.scale *= OPTION_SCALE;
    ECS::registry<MenuButton>.emplace(collectiblesEntity, ButtonEventType::COLLECTIBLES);
    ECS::registry<StartMenuTag>.emplace(collectiblesEntity);
    buttonEntities.push_back(collectiblesEntity);

    // TODO: enable for M4
	// load save button
//    auto loadSaveEntity = ECS::Entity();
//    ECS::registry<Text>.insert(
//            loadSaveEntity,
//            Text("Load save", ABEEZEE_REGULAR, { 650.0f, 450.0f })
//    );
//    Text& loadSaveText = ECS::registry<Text>.get(loadSaveEntity);
//    loadSaveText.colour = DEFAULT_COLOUR;
//    loadSaveText.scale *= OPTION_SCALE;
//    ECS::registry<MenuButton>.emplace(loadSaveEntity, ButtonEventType::LOAD_SAVE);
//    ECS::registry<StartMenuTag>.emplace(loadSaveEntity);
//    buttonEntities.push_back(loadSaveEntity);

    // clear data button
    auto clearCollectibleDataEntity = ECS::Entity();
    ECS::registry<Text>.insert(
            clearCollectibleDataEntity,
            Text("Clear collectible data", ABEEZEE_REGULAR, { 700.0f, 500.0f })
    );
    Text& clearCollectibleDataText = ECS::registry<Text>.get(clearCollectibleDataEntity);
    clearCollectibleDataText.colour = DEFAULT_COLOUR;
    clearCollectibleDataText.scale *= SUB_OPTION_SCALE;
    ECS::registry<MenuButton>.emplace(clearCollectibleDataEntity, ButtonEventType::CLEAR_COLLECT_DATA);
    ECS::registry<StartMenuTag>.emplace(clearCollectibleDataEntity);
    buttonEntities.push_back(clearCollectibleDataEntity);

}

void StartMenu::removeEntities()
{
	for (ECS::Entity entity : ECS::registry<StartMenuTag>.entities)
	{
		ECS::ContainerInterface::remove_all_components_of(entity);
	}
}

void StartMenu::exit()
{
	removeEntities();
	resetButtons();
	notify(Event(Event::MENU_CLOSE, Event::MenuType::START_MENU));
}

// On key callback
// Check out https://www.glfw.org/docs/3.3/input_guide.html
void StartMenu::on_key(int key, int, int action, int /*mod*/)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_DOWN:
			selectNextButton();
			break;
		case GLFW_KEY_UP:
			selectPreviousButton();
			break;
		case GLFW_KEY_ENTER:
			selectedKeyEvent();
			break;
		}
	}
}

void StartMenu::on_mouse_button(int button, int action, int /*mods*/)
{
	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
		selectedKeyEvent();
}

void StartMenu::on_mouse_move(vec2 mouse_pos)
{
	// remove keyboard-selected button
	activeButtonIndex = -1;

	auto& buttonContainer = ECS::registry<MenuButton>;
	auto& textContainer = ECS::registry<Text>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];
		ECS::Entity buttonEntity = buttonContainer.entities[i];
		Text& buttonText = textContainer.get(buttonEntity);

		// check whether button is being hovered over
		button.selected = mouseover(vec2(buttonText.position.x, buttonText.position.y), buttonScale, mouse_pos);
		// track for deselect on key press
		if (button.selected)
		{
			activeButtonEntity = buttonEntity;

			// find and set index of this active button in the button entities vector
			for (int j = 0; j < buttonEntities.size(); j++)
			{
				if (buttonEntity.id == buttonEntities[j].id)
					activeButtonIndex = j;
			}
		}
	}
}

void StartMenu::selectedKeyEvent()
{
	auto& buttonContainer = ECS::registry<MenuButton>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];

		// perform action for button being hovered over (and pressed)
		if (button.selected && !button.disabled)
		{
			switch (button.event)
			{
			case ButtonEventType::START_GAME:
				// exit menu and start game
				exit();
				break;
			case ButtonEventType::SELECT_LEVEL:
				// don't overlay level select on top (text render order issues; always on top)
				removeEntities();
				resetButtons();
				notify(Event(Event::MENU_OPEN, Event::LEVEL_SELECT));
				break;
			case ButtonEventType::COLLECTIBLES:
                removeEntities();
                resetButtons();
                notify(Event(Event::MENU_OPEN, Event::COLLECTIBLES_MENU));
			    break;
			case ButtonEventType::CLEAR_COLLECT_DATA:
                notify(Event(Event::CLEAR_COLLECTIBLES));
			    break;
			default:
				break;
			}
		}
	}
}

void StartMenu::updateDisabled(MenuButton &button)
{
    if ((button.event == ButtonEventType::CLEAR_COLLECT_DATA) || (button.event == ButtonEventType::COLLECTIBLES))
    {
        button.disabled = ECS::registry<Inventory>.components[0].collectibles.empty();
    }
}
