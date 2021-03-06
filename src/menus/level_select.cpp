#include "level_select.hpp"
#include "text.hpp"
#include "tiles/tiles.hpp"
#include "level_loader.hpp"
#include "spider.hpp"

#include <fstream>
#include <../ext/nlohmann_json/single_include/nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


const vec3 PREVIEW_BACKGROUND_COLOUR = { 0.007f, 0.059f, 0.012f };
// vertical spacing between title and preview
const float LEVEL_TITLE_SPACING = LevelLoader::previewScale / 2;
const vec2 SCALED_DIMENSIONS = LevelLoader::previewDimensions * LevelLoader::previewScale;
// level name size (for buttons, since you can't tell based on the Text component)
const vec2 TEXT_BUTTON_SCALE = { LevelLoader::previewDimensions.x * LevelLoader::previewScale, 2 * LevelLoader::previewScale };

LevelSelect::LevelSelect(GLFWwindow& window) : Menu(window)
{
	setActive();
}

void LevelSelect::setActive()
{
	Menu::setActive();
	loadEntities();
}

void LevelSelect::step(vec2 /*window_size_in_game_units*/)
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

void LevelSelect::loadEntities()
{
	// a big bad
	ECS::Entity spider = Spider::createSpider({ 1100, 400 });
	Motion& motion = ECS::registry<Motion>.get(spider);
	motion.scale *= 10;
	ECS::registry<ShadedMeshRef>.get(spider).renderBucket = RenderBucket::BACKGROUND_2; // render spider behind levels
	ECS::registry<LevelSelectTag>.emplace(spider);

	// level previews
	std::vector<std::string> levelNames = { "demo.json", "demo-2.json", "level-1.json"};
	vec2 previewSpacing = LevelLoader::previewScale * LevelLoader::previewDimensions + 2.f * vec2(LevelLoader::previewScale, 2.f * LevelLoader::previewScale);
	vec2 previewOffset = vec2(100.f, 100.f);

	int levelIndex = 0;
	for (int y = 0; y < 3; y++)
	{
		// reset preview offset x component
		previewOffset.x = 100.f;

		for (int x = 0; x < 3; x++)
		{
			// check if no more levels to display
			if (levelIndex >= levelNames.size())
				return;

			LevelLoader::previewLevel(levelNames[levelIndex], previewOffset);

			// draw border around preview
			createPreviewBackground(previewOffset + SCALED_DIMENSIONS / 2.f, SCALED_DIMENSIONS);

			// access level file
			std::ifstream i(levels_path(levelNames[levelIndex]));
			json level = json::parse(i);

			const auto ABEEZEE_REGULAR = Font::load("data/fonts/abeezee/ABeeZee-Regular.otf");
			const auto VIGA_REGULAR = Font::load("data/fonts/viga/Viga-Regular.otf");

			// display level name
			auto levelEntity = ECS::Entity();
			ECS::registry<Text>.insert(
				levelEntity,
				Text(level["name"], ABEEZEE_REGULAR, previewOffset)
			);
			Text& levelText = ECS::registry<Text>.get(levelEntity);
			levelText.colour = DEFAULT_COLOUR;
			levelText.scale = LEVEL_NAME_SCALE;
			levelText.position.y -= LEVEL_TITLE_SPACING; // shift text up for spacing between it and level preview
			ECS::registry<MenuButton>.emplace(levelEntity, ButtonEventType::LOAD_LEVEL);
			ECS::registry<LevelSelectTag>.emplace(levelEntity);
			buttonEntities.push_back(levelEntity);

			// shift pos to next column
			previewOffset.x += previewSpacing.x;
			levelIndex++;
		}
		// shift pos to next row
		previewOffset.y += previewSpacing.y;
	}
}

void LevelSelect::removeEntities()
{
	for (ECS::Entity entity : ECS::registry<LevelSelectTag>.entities)
		ECS::ContainerInterface::remove_all_components_of(entity);
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
		case GLFW_KEY_3:
			notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::LVL_1));
			notify(Event(Event::EventType::MENU_CLOSE_ALL));
			break;
		}
	}
}

void LevelSelect::on_mouse_button(int button, int action, int /*mods*/)
{
	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
		selectedKeyEvent();
}

void LevelSelect::on_mouse_move(vec2 mouse_pos)
{
	auto& buttonContainer = ECS::registry<MenuButton>;
	auto& textContainer = ECS::registry<Text>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];
		ECS::Entity buttonEntity = buttonContainer.entities[i];
		Text& buttonText = textContainer.get(buttonEntity);

		// check whether button is being hovered over
		vec2 buttonPos;
		vec2 buttonScale;
		if (button.event == LOAD_LEVEL)
		{
			//	adjust the button pos and scale to include the preview
			buttonPos = vec2(buttonText.position.x, buttonText.position.y + LEVEL_TITLE_SPACING + SCALED_DIMENSIONS.y);
			buttonScale = TEXT_BUTTON_SCALE + vec2(0.f, LEVEL_TITLE_SPACING + SCALED_DIMENSIONS.y);
		}
		else
		{
			buttonPos = vec2(buttonText.position.x, buttonText.position.y);
			buttonScale = TEXT_BUTTON_SCALE;
		}
		button.selected = mouseover(buttonPos, buttonScale, mouse_pos);
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

void LevelSelect::selectedKeyEvent()
{
	auto& buttonContainer = ECS::registry<MenuButton>;
	for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
	{
		MenuButton& button = buttonContainer.components[i];

		// perform action for button being hovered over (and pressed)
		if (button.selected)
		{
			switch (button.event)
			{
			case ButtonEventType::LOAD_LEVEL:
				switch (activeButtonIndex)
				{
				case 0:
					notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::DEMO));
					notify(Event(Event::EventType::MENU_CLOSE_ALL));
					break;
				case 1:
					notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::DEMO_2));
					notify(Event(Event::EventType::MENU_CLOSE_ALL));
					break;
				case 2:
					notify(Event(Event::EventType::LOAD_LEVEL, Event::Level::LVL_1));
					notify(Event(Event::EventType::MENU_CLOSE_ALL));
					break;
				}
				break;
			default:
				break;
			}
		}
	}
}

void LevelSelect::createPreviewBackground(vec2 position, vec2 scale)
{
	auto entity = ECS::Entity();

	std::string key = "preview_border";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0) {
		// create a procedural circle
		constexpr float z = -0.1f;

		// Corner points
		ColoredVertex v;
		v.position = { -0.5,-0.5,z };
		v.color = PREVIEW_BACKGROUND_COLOUR;
		resource.mesh.vertices.push_back(v);
		v.position = { -0.5,0.5,z };
		v.color = PREVIEW_BACKGROUND_COLOUR;
		resource.mesh.vertices.push_back(v);
		v.position = { 0.5,0.5,z };
		v.color = PREVIEW_BACKGROUND_COLOUR;
		resource.mesh.vertices.push_back(v);
		v.position = { 0.5,-0.5,z };
		v.color = PREVIEW_BACKGROUND_COLOUR;
		resource.mesh.vertices.push_back(v);

		// Two triangles
		resource.mesh.vertex_indices.push_back(0);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(3);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(2);
		resource.mesh.vertex_indices.push_back(3);

		RenderSystem::createColoredMesh(resource, "colored_mesh");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::BACKGROUND);

	// Create motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	ECS::registry<LevelSelectTag>.emplace(entity);
}
