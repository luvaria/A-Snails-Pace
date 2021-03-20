#pragma once

#include "tiny_ecs.hpp"
#include "common.hpp"

struct Collectible
{
    static ECS::Entity createCollectible(vec2 position, int id);
    static void equip(ECS::Entity host, int id);
    static void unequip(ECS::Entity host);
    int id = -1;
};

// component to associate an entity with it's equipped collectible entity
struct Equipped
{
    Equipped(ECS::Entity collectible) : collectible(collectible) {}
    static void moveEquippedWithHost();
    ECS::Entity collectible;
};