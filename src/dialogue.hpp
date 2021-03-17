# pragma once

#include "common.hpp"
#include "observer.hpp"
#include "subject.hpp"

#include <queue>

struct Dialogue
{
	// -1: not a line of text
	int lineNumber = -1;
	Dialogue();
	Dialogue(int lineNumber);
};

class DialogueSystem : public Observer, public Subject
{
public:
	// constructor
	DialogueSystem(vec2 window_size_in_game_units);

	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// sets new dialogue (sets active flag)
	void setDialogue(std::string dialogue);
	// clears old dialogue (clears active flag)
	void clearDialogue();
	// updates and displays current page
	void displayCurrent(float elapsed_ms);
	// moves to next page of dialogue
	void nextPage();

private:
	bool isActive;
	std::queue<std::vector<std::string>> dialogueQueue;

	vec2 window_size_in_game_units;

	void onNotify(Event event);

	void registerNPCs();

	// line-wraps dialogue and splits into pages, adding to the dialogue queue
	void wrapDialogue(std::string dialogue);

	void createTextLines();
	void createTextBox();

	void clearDialogueEntities();
};
