#include "collectible_menu.hpp"
#include "collectible.hpp"
#include "text.hpp"

const float BOX_LENGTH = 150.f;
const float BOX_OFFSET = BOX_LENGTH/4.f;
const vec3 BUTTON_BACKGROUND_COLOUR = { 0.f, 0.f, 0.502f }; // navy blue
const vec3 BUTTON_EQUIPPED_COLOUR = { 0.93f, 0.8f, 0.68f }; // golden
const vec3 BUTTON_SELECTED_COLOUR = { 0.26f, 0.39f, 0.50f }; // blue whale

CollectMenu::CollectMenu(GLFWwindow &window) : Menu(window)
{
    setActive();
}

void CollectMenu::setActive()
{
    Menu::setActive();
    loadEntities();
}

void CollectMenu::step(vec2 /*window_size_in_game_units*/)
{
    const auto ABEEZEE_REGULAR = Font::load("data/fonts/abeezee/ABeeZee-Regular.otf");
    const auto ABEEZEE_ITALIC = Font::load("data/fonts/abeezee/ABeeZee-Italic.otf");

    // update button colour based on mouseover
    auto& buttonContainer = ECS::registry<MenuButton>;
    for (unsigned int i = 0; i < buttonEntities.size(); i++)
    {
        ECS::Entity buttonEntity = buttonEntities[i];
        MenuButton& button = buttonContainer.get(buttonEntity);

        if (collectible_ids[i] == ECS::registry<Inventory>.components[0].equipped)
        {
            updateButtonBackgroundColor(buttonEntity, BUTTON_EQUIPPED_COLOUR);
        }
        else if (button.selected)
        {
            updateButtonBackgroundColor(buttonEntity, BUTTON_SELECTED_COLOUR);
        }
        else
        {
            updateButtonBackgroundColor(buttonEntity, BUTTON_BACKGROUND_COLOUR);
        }
    }
}

void CollectMenu::loadEntities()
{
    unsigned cols = 4;
    // 1-indexed for the math
    unsigned currCol = 1;
    unsigned currRow = 1;
    for (CollectId id : ECS::registry<Inventory>.components[0].collectibles)
    {
        vec2 position = { (BOX_OFFSET + BOX_LENGTH/2) * currCol, (BOX_OFFSET + BOX_LENGTH/2) * currRow };
        auto collectible = Collectible::createCollectible(position, id);
        Motion& motion = ECS::registry<Motion>.get(collectible);
        motion.scale = normalize(motion.scale) * BOX_LENGTH;
        ECS::registry<CollectMenuTag>.emplace(collectible);

        ECS::Entity entity = createButtonBackground(position, { BOX_LENGTH, BOX_LENGTH });
        ECS::registry<MenuButton>.emplace(entity,ButtonEventType::EQUIP_COLLECTIBLE);
        buttonEntities.push_back(entity);
        collectible_ids.push_back(id);

        currCol++;
        if (currCol >= cols)
        {
            currCol = 0;
            currRow++;
        }
    }
}

void CollectMenu::removeEntities()
{
    for (ECS::Entity entity : ECS::registry<CollectMenuTag>.entities)
    {
        ECS::ContainerInterface::remove_all_components_of(entity);
    }
}

void CollectMenu::exit()
{
    removeEntities();
    resetButtons();
    notify(Event(Event::MENU_CLOSE, Event::MenuType::COLLECTIBLES_MENU));
}

void CollectMenu::on_key(int key, int, int action, int /*mod*/)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                // exit collect select (go back to start menu)
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
        }
    }
}

void CollectMenu::on_mouse_button(int button, int action, int /*mods*/)
{
    if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
        selectedKeyEvent();
}

void CollectMenu::on_mouse_move(vec2 mouse_pos)
{
    // remove keyboard-selected button
    activeButtonIndex = -1;

    auto& buttonContainer = ECS::registry<MenuButton>;
    for (unsigned int i = 0; i < buttonContainer.components.size(); i++)
    {
        MenuButton& button = buttonContainer.components[i];
        ECS::Entity buttonEntity = buttonContainer.entities[i];
        auto& buttonMotion = ECS::registry<Motion>.get(buttonEntity);

        // check whether button is being hovered over
        vec2 buttonPos = { buttonMotion.position.x - BOX_LENGTH/2, buttonMotion.position.y + BOX_LENGTH/2 };
        vec2 buttonScale = buttonMotion.scale;
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

void CollectMenu::selectedKeyEvent() {
    auto &buttonContainer = ECS::registry<MenuButton>;
    for (unsigned int i = 0; i < buttonContainer.components.size(); i++) {
        MenuButton &button = buttonContainer.components[i];

        // perform action for button being hovered over (and pressed)
        if (button.selected) {
            switch (button.event) {
                case ButtonEventType::EQUIP_COLLECTIBLE:
                    notify(Event(Event::EventType::EQUIP_COLLECTIBLE, collectible_ids[activeButtonIndex]));
                    break;
                default:
                    break;
            }
        }
    }
}

ECS::Entity CollectMenu::createButtonBackground(vec2 const& position, vec2 const& scale)
{
    auto entity = ECS::Entity();

    std::string key = "collect_button";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0) {
        // create a procedural circle
        constexpr float z = -0.1f;
        vec3 color = {1.f,1.f,1.f};

        // Corner points
        ColoredVertex v;
        v.position = { -0.5,-0.5,z };
        v.color = color;
        resource.mesh.vertices.push_back(v);
        v.position = { -0.5,0.5,z };
        v.color = color;
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5,0.5,z };
        v.color = color;
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5,-0.5,z };
        v.color = color;
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

    ECS::registry<CollectMenuTag>.emplace(entity);

    return entity;
}

void CollectMenu::updateButtonBackgroundColor(ECS::Entity buttonBackground, vec3 const& color)
{
    ECS::registry<ShadedMeshRef>.get(buttonBackground).reference_to_cache->texture.color = color;
}