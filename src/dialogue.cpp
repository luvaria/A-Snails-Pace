 #include "dialogue.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "text.hpp"
#include "npc.hpp"

const int MS_PER_CHAR = 10;
float dialogue_displayed_ms = 0.f;

Dialogue::Dialogue() {}
Dialogue::Dialogue(int lineNumber) : lineNumber(lineNumber) {}

DialogueSystem::DialogueSystem(vec2 window_size_in_game_units) :
	isActive(false), window_size_in_game_units(window_size_in_game_units) {}

void DialogueSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	if (isActive)
	{
		displayCurrent(elapsed_ms);
	}
	else
	{
		clearDialogueEntities();
	}
}

void DialogueSystem::setDialogue(std::string dialogue)
{
	clearDialogue();
	isActive = true;
	wrapDialogue(dialogue);
	dialogue_displayed_ms = 0.f;

	createTextLines();
	createTextBox();
}

void DialogueSystem::clearDialogue()
{
	isActive = false;
	while (!dialogueQueue.empty())
		dialogueQueue.pop();
	clearDialogueEntities();
}

void DialogueSystem::displayCurrent(float elapsed_ms)
{
	dialogue_displayed_ms += elapsed_ms;

	const auto FANTASQUE_SANS_MONO_REGULAR = Font::load(FANTASQUE_SANS_MONO_REGULAR_PATH);

	int chars_to_display = dialogue_displayed_ms / MS_PER_CHAR;
	int chars_displayed = 0;
	for (int i = 0; i < dialogueQueue.front().size(); i++)
	{
		std::string line = dialogueQueue.front()[i];
		if (line.length() <= chars_to_display - chars_displayed)
		{
			chars_displayed += line.length();
		}
		else
		{
			line = line.substr(0, chars_to_display - chars_displayed);
			chars_displayed = chars_to_display;
		}

		// find correct text line to update
		for (int j = 0; j < ECS::registry<Dialogue>.components.size(); j++)
		{
			Dialogue& dialogue = ECS::registry<Dialogue>.components[j];

			if (dialogue.lineNumber == i)
			{
				ECS::Entity dialogueEntity = ECS::registry<Dialogue>.entities[j];
				Text& text = ECS::registry<Text>.get(dialogueEntity);
				text.content = line;
				break;
			}
		}

		if (chars_displayed == chars_to_display)
			break;
	}
}

void DialogueSystem::nextPage()
{
	clearDialogueEntities();

	if (!dialogueQueue.empty())
	{
		createTextLines();
		createTextBox();
		dialogueQueue.pop();
		dialogue_displayed_ms = 0.f;
	}

	if (dialogueQueue.empty())
		isActive = false;
}

void DialogueSystem::onNotify(Event event)
{
	switch (event.type)
	{
	case Event::START_DIALOGUE:
		setDialogue(event.dialogue);
		break;
	case Event::NEXT_DIALOGUE:
		nextPage();
		if (!isActive)
		{
			// tell NPC to move to next dialogue (pages exhausted)
			notify(event);
		}
		break;
	// game unpaused
	case Event::RESUME_DIALOGUE:
		createTextLines();
		createTextBox();
		break;
	case Event::END_DIALOGUE:
		clearDialogue();
		break;
	case Event::LEVEL_LOADED:
		clearDialogue();
		registerNPCs();
		break;
	}
}

void DialogueSystem::registerNPCs()
{
	this->clearObservers();

	for (NPC& npc : ECS::registry<NPC>.components)
	{
		npc.addObserver(this);
		this->addObserver(&npc);
	}
}

void DialogueSystem::wrapDialogue(std::string dialogue)
{
	const int charsPerLine = window_size_in_game_units.x * (1 - 2 * DIALOGUE_POS.x) / (VERTICAL_TEXT_SIZE * DIALOGUE_SCALE * FANTASQUE_SANS_MONO_REGULAR_WIDTH);
	const int linesPerPage = window_size_in_game_units.y * (1 - DIALOGUE_POS.y - 0.5f * DIALOGUE_POS.x) / (VERTICAL_TEXT_SIZE * DIALOGUE_SCALE + LINE_SPACING);
	
	std::vector<std::string> curPage;
	int lineStartIndex = 0;
	int lineEndIndex = 0;
	int lineCount = 0;
	for (int i = 0; i < dialogue.length(); i++)
	{
		// words are separated by spaces
		//	no spaces are required at the end of the dialogue string
		if (dialogue[i] == ' ' || i == (dialogue.length() - 1))
		{
			// word fits in current line
			if ((i - lineStartIndex) <= charsPerLine)
			{
				lineEndIndex = i;
			}
			// word does not fit; change line
			else
			{
				// new page needed
				if (lineCount == linesPerPage)
				{
					dialogueQueue.push(curPage);
					curPage = {};
					lineCount = 0;
				}

				// append current line
				curPage.push_back(dialogue.substr(lineStartIndex, lineEndIndex - lineStartIndex));
				lineEndIndex++; // skip space
				lineStartIndex = lineEndIndex;
				lineCount++;
			}
		}
	}
	// leftover text of last line/page
	lineEndIndex = dialogue.length();
	curPage.push_back(dialogue.substr(lineStartIndex, lineEndIndex - lineStartIndex));
	dialogueQueue.push(curPage);
}

void DialogueSystem::createTextLines()
{
	const auto FANTASQUE_SANS_MONO_REGULAR = Font::load(FANTASQUE_SANS_MONO_REGULAR_PATH);


	for (int i = 0; i < dialogueQueue.front().size(); i++)
	{
		ECS::Entity entity = ECS::Entity();
		ECS::registry<Dialogue>.emplace(entity, i);
		ECS::registry<Overlay>.emplace(entity);
		ECS::registry<Text>.insert(
			entity,
			Text("", FANTASQUE_SANS_MONO_REGULAR, DIALOGUE_POS * window_size_in_game_units)
		);
		Text& text = ECS::registry<Text>.get(entity);
		text.colour = OVERLAY_COLOUR;
		text.scale = DIALOGUE_SCALE;
		// line spacing
		text.position.y += i * (VERTICAL_TEXT_SIZE * DIALOGUE_SCALE + window_size_in_game_units.y * LINE_SPACING);
	}
}

void DialogueSystem::createTextBox()
{
	// hijacked from DebugSystem createLine()

	auto entity = ECS::Entity();

	std::string key = "text_box";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0) {
		// create a procedural circle
		constexpr float z = -0.1f;

		// Corner points
		ColoredVertex v;
		v.color = DEFAULT_COLOUR;
		v.position = { -0.5, -0.5, z };
		resource.mesh.vertices.push_back(v);
		v.position = { -0.5, 0.5, z };
		resource.mesh.vertices.push_back(v);
		v.position = { 0.5, 0.5, z };
		resource.mesh.vertices.push_back(v);
		v.position = { 0.5, -0.5, z };
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
	ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::FOREGROUND);
	ECS::registry<Dialogue>.emplace(entity);
	ECS::registry<Overlay>.emplace(entity);

	// Create motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = { window_size_in_game_units.x * 0.5f, window_size_in_game_units.y * 0.8f };
	motion.scale = { window_size_in_game_units.x * 0.9f, window_size_in_game_units.y * 0.325f };;
}

void DialogueSystem::clearDialogueEntities()
{
	for (ECS::Entity entity : ECS::registry<Dialogue>.entities)
		ECS::ContainerInterface::remove_all_components_of(entity);
}
