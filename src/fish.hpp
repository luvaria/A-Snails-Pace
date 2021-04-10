#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "load_save.hpp"

struct Fish
{
    struct Move {
        bool direction;
        bool hasMoved;
        void setFromJson(json const& saved);
        void writeToJson(json& toSave);
    };

    // Creates all the associated render resources and default transform
	static ECS::Entity createFish(Motion motion, ECS::Entity entity = ECS::Entity());
	static ECS::Entity createFish(vec2 position, ECS::Entity entity = ECS::Entity());
};


