# pragma once

#include "common.hpp"
#include "render.hpp"
#include "subject.hpp"
#include "tiles/tiles.hpp"

// stlib
#include <iostream>
#include <functional>

enum ButtonEventType { NO_ACTION, START_GAME, SELECT_LEVEL, NEXT_LEVEL, COLLECTIBLES, /*LOAD_SAVE,*/ CLEAR_COLLECT_DATA,
        LOAD_LEVEL, EQUIP_COLLECTIBLE, QUIT_GAME, SET_VOLUME };

// button tag
struct MenuButton
{
	// true when mouse is hovering over
	bool selected;
	// true if cannot be pressed
	bool disabled;
	ButtonEventType event = ButtonEventType::NO_ACTION;

	MenuButton(ButtonEventType e) : selected(false), disabled(false), event(e) {}
};

class Menu : public Subject
{
protected:
	GLFWwindow& window;

	Menu(GLFWwindow& window) : window(window), activeButtonIndex(-1) {}

	// returns true if mouse is hovering over button, false otherwise
	// text buttons
	bool mouseover(vec2 buttonPos, vec2 buttonScale, vec2 mousePos)
	{
		float upperBound = buttonPos.y - buttonScale.y;
		float lowerBound = buttonPos.y;
		float leftBound = buttonPos.x;
		float rightBound = buttonPos.x + buttonScale.x;

		return (mousePos.y >= upperBound && mousePos.y <= lowerBound && mousePos.x >= leftBound && mousePos.x <= rightBound);
	}
	// Motion-based buttons
	bool mouseover(Motion& motion, vec2 mousePos)
	{
		float upperBound = motion.position.y - abs(motion.scale.y) / 2;
		float lowerBound = motion.position.y + abs(motion.scale.y) / 2;
		float leftBound = motion.position.x - abs(motion.scale.x) / 2;
		float rightBound = motion.position.x + abs(motion.scale.x) / 2;

		return (mousePos.y >= upperBound && mousePos.y <= lowerBound && mousePos.x >= leftBound && mousePos.x <= rightBound);
	}

	// might be able to use the ECS button container, but I'm not sure about the order
	std::vector<ECS::Entity> buttonEntities;
	// for iterating through the buttons vector
	int activeButtonIndex;
	// last mouse hover button, for deselecting on key press. assumes only one button can be active at a time (none overlap)
	ECS::Entity activeButtonEntity;
	void resetButtons()
	{
		buttonEntities.clear();
		activeButtonIndex = -1;
	}
	void selectNextButton()
	{
		// no buttons
		if (buttonEntities.size() == 0)
			return;

		// deselect previous button
		if (ECS::registry<MenuButton>.has(activeButtonEntity))
		{
			MenuButton& activeButton = ECS::registry<MenuButton>.get(activeButtonEntity);
			activeButton.selected = false;
		}

		// none active or looping around, so set first button as active
		if (activeButtonIndex == -1 || activeButtonIndex == buttonEntities.size() - 1)
			activeButtonIndex = 0;
		else
			activeButtonIndex++;

		activeButtonEntity = buttonEntities[activeButtonIndex];
		ECS::registry<MenuButton>.get(activeButtonEntity).selected = true;
	}
	void selectPreviousButton()
	{
		// no buttons
		if (buttonEntities.size() == 0)
			return;

		// deselect previous button
		if (ECS::registry<MenuButton>.has(activeButtonEntity))
		{
			MenuButton& activeButton = ECS::registry<MenuButton>.get(activeButtonEntity);
			activeButton.selected = false;
		}

		if (activeButtonIndex == -1 || activeButtonIndex == 0)	
			activeButtonIndex = static_cast<int>(buttonEntities.size()) - 1; // none active or looping around, so set last button as active
		else
			activeButtonIndex--;

		activeButtonEntity = buttonEntities[activeButtonIndex];
		ECS::registry<MenuButton>.get(activeButtonEntity).selected = true;
	}

public:
	virtual ~Menu() {}
	virtual void setActive()
	{
		setGLFWCallbacks();
	}
	void setInactive() { this->removeEntities(); }
	virtual void step(vec2 window_size_in_game_units) = 0;

private:
	virtual void loadEntities() = 0;
	virtual void removeEntities() = 0;
	virtual void exit() = 0;

	// Input callback functions
	virtual void on_key(int key, int, int action, int mod) = 0;
	virtual void on_mouse_move(vec2 mouse_pos) = 0;
	virtual void on_mouse_button(int button, int action, int /*mods*/) = 0;

	virtual void selectedKeyEvent() = 0;

	void setGLFWCallbacks()
	{
		// Setting callbacks to member functions (that's why the redirect is needed)
		// Input is handled using GLFW, for more info see
		// http://www.glfw.org/docs/latest/input_guide.html
		glfwSetWindowUserPointer(&window, this);
		auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((Menu*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
		auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((Menu*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
		auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((Menu*)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };

		glfwSetKeyCallback(&window, key_redirect);
		glfwSetCursorPosCallback(&window, cursor_pos_redirect);
		glfwSetMouseButtonCallback(&window, mouse_button_redirect);
	}
};
