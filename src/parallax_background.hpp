#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include "observer.hpp"

class BackgroundSystem : public Observer
{
public:
	BackgroundSystem(vec2 window_size_in_game_units) : window_size_in_game_units(window_size_in_game_units) {}

	void addBackground(std::string bgName, Parallax::Layer layer);
	void addBackgrounds();
	void removeBackgrounds();

	void step();

	void onNotify(Event event);

private:
	// quadruplets of background entities (for infinite scrolling)
	//	first pair is left-right scrolling
	//	second pair is up-down scrolling
	std::vector<std::pair<std::pair<ECS::Entity, ECS::Entity>, std::pair<ECS::Entity, ECS::Entity>>> backgrounds;

	vec2 window_size_in_game_units;
};
