#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Particle
{
    static ECS::Entity createWeatherParticle(std::string particleName, ECS::Entity entity, vec2 window_size_in_game_units);
    static ECS::Entity createParticle(Motion motion, ECS::Entity entity = ECS::Entity());
    static void setP1Motion(Motion& mot);
    static void setP2Motion(Motion& mot);
    static void setP3Motion(Motion& mot);
    static float timer;
};
struct BlurParticle
{
    ECS::Entity entity;
    static bool canCreateNew;
    static float timer;
};
struct WeatherParticle
{
    ECS::Entity entity;
    static float nextSpawn;
    static float count;
    static float timer;
};
struct RejectedStage2
{
    ECS::Entity entity;
};
struct RejectedStage3
{
    ECS::Entity entity;
};
