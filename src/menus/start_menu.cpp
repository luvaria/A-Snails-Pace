#include "start_menu.hpp"
#include "text.hpp"
#include "snail.hpp"
#include "load_save.hpp"
#include "../tiles/tiles.hpp"
#include "../tiles/wall.hpp"
#include "../tiles/vine.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

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

		if (!textContainer.has(buttonEntity))
		{
			continue;
		}

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
	Motion& snailMotion = ECS::registry<Motion>.get(snail);
	snailMotion.scale *= 6;
	ECS::registry<StartMenuTag>.emplace(snail);

	// ensure snail is not red from quitting during DeathTimer
	auto& texmesh = *ECS::registry<ShadedMeshRef>.get(snail).reference_to_cache;
	texmesh.texture.color = { 1, 1, 1 };

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

    // continue from last save
    auto loadSaveEntity = ECS::Entity();
    ECS::registry<Text>.insert(
            loadSaveEntity,
            Text("Continue", ABEEZEE_REGULAR, { 650.0f, 400.0f })
    );
    Text& loadSaveText = ECS::registry<Text>.get(loadSaveEntity);
    loadSaveText.colour = DEFAULT_COLOUR;
    loadSaveText.scale *= OPTION_SCALE;
    ECS::registry<MenuButton>.emplace(loadSaveEntity, ButtonEventType::LOAD_SAVE);
    ECS::registry<StartMenuTag>.emplace(loadSaveEntity);
    buttonEntities.push_back(loadSaveEntity);

	// level select button
	auto selectLevelEntity = ECS::Entity();
	ECS::registry<Text>.insert(
		selectLevelEntity,
		Text("Select level", ABEEZEE_REGULAR, { 650.0f, 450.0f })
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
            Text("Collectibles", ABEEZEE_REGULAR, { 650.0f, 500.0f })
    );
    Text& collectiblesText = ECS::registry<Text>.get(collectiblesEntity);
    collectiblesText.colour = DEFAULT_COLOUR;
    collectiblesText.scale *= OPTION_SCALE;
    ECS::registry<MenuButton>.emplace(collectiblesEntity, ButtonEventType::COLLECTIBLES);
    ECS::registry<StartMenuTag>.emplace(collectiblesEntity);
    buttonEntities.push_back(collectiblesEntity);


    // clear data button
    auto clearCollectibleDataEntity = ECS::Entity();
    ECS::registry<Text>.insert(
            clearCollectibleDataEntity,
            Text("Clear collectible data", ABEEZEE_REGULAR, { 700.0f, 550.0f })
    );
    Text& clearCollectibleDataText = ECS::registry<Text>.get(clearCollectibleDataEntity);
    clearCollectibleDataText.colour = DEFAULT_COLOUR;
    clearCollectibleDataText.scale *= SUB_OPTION_SCALE;
    ECS::registry<MenuButton>.emplace(clearCollectibleDataEntity, ButtonEventType::CLEAR_COLLECT_DATA);
    ECS::registry<StartMenuTag>.emplace(clearCollectibleDataEntity);
    buttonEntities.push_back(clearCollectibleDataEntity);

	// volume slider
	ECS::Entity volumeSliderEntity = ECS::Entity();

	std::string sliderKey = "volume_slider";
	ShadedMesh& sliderResource = cache_resource(sliderKey);
	if (sliderResource.effect.program.resource == 0) {
		constexpr float z = -0.1f;

		ColoredVertex v;
		v.color = DEFAULT_COLOUR;
		v.position = { -0.5, -0.5, z };
		sliderResource.mesh.vertices.push_back(v);
		v.position = { -0.5, 0.5, z };
		sliderResource.mesh.vertices.push_back(v);
		v.position = { 0.5, 0.5, z };
		sliderResource.mesh.vertices.push_back(v);
		v.position = { 0.5, -0.5, z };
		sliderResource.mesh.vertices.push_back(v);

		sliderResource.mesh.vertex_indices.push_back(0);
		sliderResource.mesh.vertex_indices.push_back(1);
		sliderResource.mesh.vertex_indices.push_back(3);
		sliderResource.mesh.vertex_indices.push_back(1);
		sliderResource.mesh.vertex_indices.push_back(2);
		sliderResource.mesh.vertex_indices.push_back(3);

		RenderSystem::createColoredMesh(sliderResource, "colored_mesh");
	}

	ECS::registry<ShadedMeshRef>.emplace(volumeSliderEntity, sliderResource, RenderBucket::OVERLAY);

	Motion& volMotion = ECS::registry<Motion>.emplace(volumeSliderEntity);
	volMotion.angle = 0.f;
	volMotion.velocity = { 0, 0 };
	volMotion.position = { 100, 50 };
	volMotion.scale = { 100, 10 };

	ECS::registry<MenuButton>.emplace(volumeSliderEntity, ButtonEventType::SET_VOLUME);
	ECS::registry<StartMenuTag>.emplace(volumeSliderEntity);

	// slider feedback
	float curVol = static_cast<float>(max(Mix_Volume(-1, -1), Mix_VolumeMusic(-1))) / MIX_MAX_VOLUME;
	ECS::Entity volumeIndicatorEntity = ECS::Entity();

	std::string indicatorKey = "volume_indicator";
	ShadedMesh& indicatorResource = cache_resource(indicatorKey);
	if (indicatorResource.effect.program.resource == 0) {
		constexpr float z = -0.1f;

		ColoredVertex v;
		v.color = HIGHLIGHT_COLOUR;
		v.position = { -0.5, -0.5, z };
		indicatorResource.mesh.vertices.push_back(v);
		v.position = { -0.5, 0.5, z };
		indicatorResource.mesh.vertices.push_back(v);
		v.position = { 0.5, 0.5, z };
		indicatorResource.mesh.vertices.push_back(v);
		v.position = { 0.5, -0.5, z };
		indicatorResource.mesh.vertices.push_back(v);

		indicatorResource.mesh.vertex_indices.push_back(0);
		indicatorResource.mesh.vertex_indices.push_back(1);
		indicatorResource.mesh.vertex_indices.push_back(3);
		indicatorResource.mesh.vertex_indices.push_back(1);
		indicatorResource.mesh.vertex_indices.push_back(2);
		indicatorResource.mesh.vertex_indices.push_back(3);

		RenderSystem::createColoredMesh(indicatorResource, "colored_mesh");
	}

	ECS::registry<ShadedMeshRef>.emplace(volumeIndicatorEntity, indicatorResource, RenderBucket::OVERLAY_2);

	Motion& indicatorMotion = ECS::registry<Motion>.emplace(volumeIndicatorEntity);
	indicatorMotion.angle = 0.f;
	indicatorMotion.velocity = { 0, 0 };
	indicatorMotion.scale = { 100, 10 };
	indicatorMotion.scale.x *= curVol;
	indicatorMotion.position = { 50 + indicatorMotion.scale.x / 2, 50 };

	ECS::registry<StartMenuTag>.emplace(volumeIndicatorEntity);
	ECS::registry<VolumeIndicator>.emplace(volumeIndicatorEntity);

	// volume min max icons
	// min
	ECS::Entity minVolEntity = ECS::Entity();
	std::string minVolKey = "min_volume";
	ShadedMesh& minVolResource = cache_resource(minVolKey);
	if (minVolResource.effect.program.resource == 0)
	{
		minVolResource = ShadedMesh();
		RenderSystem::createSprite(minVolResource, textures_path("min_volume.png"), "textured", false);
	}

	ECS::registry<ShadedMeshRef>.emplace(minVolEntity, minVolResource, RenderBucket::OVERLAY);

	Motion& minVolMotion = ECS::registry<Motion>.emplace(minVolEntity);
	minVolMotion.angle = 0.f;
	minVolMotion.velocity = { 0, 0 };
	minVolMotion.position = { 25, 50 };
	minVolMotion.scale = static_cast<vec2>(minVolResource.texture.size) / static_cast<vec2>(minVolResource.texture.size).y * 20.f;

	ECS::registry<MenuButton>.emplace(minVolEntity, ButtonEventType::MIN_VOLUME);
	ECS::registry<StartMenuTag>.emplace(minVolEntity);

	// max
	ECS::Entity maxVolEntity = ECS::Entity();
	std::string maxVolKey = "max_volume";
	ShadedMesh& maxVolResource = cache_resource(maxVolKey);
	if (maxVolResource.effect.program.resource == 0)
	{
		maxVolResource = ShadedMesh();
		RenderSystem::createSprite(maxVolResource, textures_path("max_volume.png"), "textured", false);
	}

	ECS::registry<ShadedMeshRef>.emplace(maxVolEntity, maxVolResource, RenderBucket::OVERLAY);

	Motion& maxVolMotion = ECS::registry<Motion>.emplace(maxVolEntity);
	maxVolMotion.angle = 0.f;
	maxVolMotion.velocity = { 0, 0 };
	maxVolMotion.position = { 175, 50 };
	maxVolMotion.scale = static_cast<vec2>(maxVolResource.texture.size) / static_cast<vec2>(maxVolResource.texture.size).y * 20.f;

	ECS::registry<MenuButton>.emplace(maxVolEntity, ButtonEventType::MAX_VOLUME);
	ECS::registry<StartMenuTag>.emplace(maxVolEntity);
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
		
		// text buttons
		if (textContainer.has(buttonEntity))
		{
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
		// volume slider
		else
		{
			Motion& motion = ECS::registry<Motion>.get(buttonEntity);
			button.selected = mouseover(motion, mouse_pos);
		}
	}
}

void StartMenu::selectedKeyEvent()
{
	auto& buttonContainer = ECS::registry<MenuButton>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		ECS::Entity buttonEntity = buttonContainer.entities[i];
		MenuButton& button = buttonContainer.components[i];

		// perform action for button being hovered over (and pressed)
		if (button.selected && !button.disabled)
		{
			double volumeRatio = 0;

			switch (button.event)
			{
			case ButtonEventType::START_GAME:
				// exit menu and start game
				exit();
				break;
			case ButtonEventType::LOAD_SAVE:
                removeEntities();
                resetButtons();
                notify(Event(Event::LOAD_SAVE));
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
				ECS::registry<Inventory>.components[0].clear();
			    break;
			case ButtonEventType::MIN_VOLUME:
				setVolume(buttonEntity, 0);
				break;
			case ButtonEventType::MAX_VOLUME:
				setVolume(buttonEntity, 1);
				break;
			case ButtonEventType::SET_VOLUME:
			{
				double xPos = 0;
				double yPos = 0;
				glfwGetCursorPos(&window, &xPos, &yPos);
				Motion& motion = ECS::registry<Motion>.get(buttonEntity);
				double volumeRatio = (xPos - (motion.position.x - abs(motion.scale.x) / 2)) / abs(motion.scale.x);
				setVolume(buttonEntity, volumeRatio);
				break;
			}
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
    else if (button.event == ButtonEventType::LOAD_SAVE)
    {
        button.disabled = !LoadSaveSystem::levelFileExists();
    }
}

void StartMenu::setVolume(ECS::Entity buttonEntity, double volumeRatio)
{
	Motion& motion = ECS::registry<Motion>.get(buttonEntity);

	ECS::Entity volumeIndicatorEntity = ECS::registry<VolumeIndicator>.entities[0];
	Motion& indicatorMotion = ECS::registry<Motion>.get(volumeIndicatorEntity);
	indicatorMotion.scale.x = volumeRatio * 100;
	indicatorMotion.position = { 50 + indicatorMotion.scale.x / 2, 50 };

	Mix_Volume(-1, volumeRatio * MIX_MAX_VOLUME);
	Mix_VolumeMusic(volumeRatio * MIX_MAX_VOLUME);
}
