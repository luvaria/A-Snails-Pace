# pragma once

#include "common.hpp"
#include "render.hpp"
#include "subject.hpp"

// stlib
#include <iostream>
#include <functional>

class Menu : public Subject
{
protected:
	GLFWwindow& window;

	Menu(GLFWwindow& window) : window(window) {}

	// returns true if mouse is hovering over button, false otherwise
	bool mouseover(vec2 buttonPos, vec2 buttonScale, vec2 mousePos)
	{
		float upperBound = buttonPos.y - buttonScale.y;
		float lowerBound = buttonPos.y;
		float leftBound = buttonPos.x;
		float rightBound = buttonPos.x + buttonScale.x;

		return (mousePos.y >= upperBound && mousePos.y <= lowerBound && mousePos.x >= leftBound && mousePos.x <= rightBound);
	}

public:
	virtual ~Menu() {}
	virtual void setActive() { setGLFWCallbacks(); }
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

enum ButtonEventType { NO_ACTION, START_GAME, SELECT_LEVEL };

// button tag
struct Button
{
	// true when mouse is hovering over
	bool mouseover = false;
	ButtonEventType event = ButtonEventType::NO_ACTION;

	Button(ButtonEventType e) : event(e) {}
};
