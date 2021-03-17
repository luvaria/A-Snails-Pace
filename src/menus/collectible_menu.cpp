#include "collectible_menu.hpp"
#include "collectible.hpp"
#include "text.hpp"

const vec2 SCALED_DIMENSIONS = { 300.f, 300.f };

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

void CollectMenu::loadEntities()
{
    unsigned cols = 4;
    unsigned currCol = 0;
    for (CollectId id : ECS::registry<Inventory>.components[0].collectibles)
    {
        Collectible::createCollectible()
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

void CollectMenu::selectedKeyEvent()
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
                    notify(Event(Event::EventType::LOAD_LEVEL, activeButtonIndex));
                    notify(Event(Event::EventType::MENU_CLOSE_ALL));
                    break;
                default:
                    break;
            }
        }
    }