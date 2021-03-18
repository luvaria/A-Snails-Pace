#include "menu.hpp"

class CollectMenu : public Menu
{
public:
    CollectMenu(GLFWwindow& window);
    void setActive();
    void step(vec2 window_size_in_game_units);

private:
    void loadEntities();
    void removeEntities();
    void exit();

    void on_key(int key, int, int action, int mod);
    void on_mouse_move(vec2 mouse_pos);
    void on_mouse_button(int button, int action, int /*mods*/);

    void selectedKeyEvent();

    // only for this subclass, may be useful for all subclasses?
    ///void updateDisabled(MenuButton& button);

    ECS::Entity createButtonBackground(vec2 const& position, vec2 const& scale, int id);
    void updateButtonBackgroundColor(ECS::Entity buttonBackground, vec3 const& color);

    // something to preserve order with buttons
    std::vector<CollectId> collectible_ids;
};

// component tag
struct CollectMenuTag {};