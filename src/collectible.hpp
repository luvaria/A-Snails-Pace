#pragma once

#include "tiny_ecs.hpp"
#include "common.hpp"

struct Collectible
{
    static ECS::Entity createCollectible(vec2 position, int id);
    static void equip(ECS::Entity host, int id);
    int id = -1;
};