#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <vector>

struct Particle
{
    
    static ECS::Entity createWeatherParticle(std::string particleName, ECS::Entity entity, vec2 window_size_in_game_units);
    static ECS::Entity createParticle(Motion motion, ECS::Entity entity = ECS::Entity());
    static ECS::Entity createWeatherChildParticle(std::string particleName, ECS::Entity entity, vec2 window_size_in_game_units);

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
    static int count;
    static float nextSpawn;
};
struct WeatherParentParticle
{
    std::vector<ECS::Entity> particles;
    ECS::Entity entity;
    static float nextSpawn;
    static float count;
    static float timer;
};
struct RejectedStages
{
    ECS::Entity entity;
    bool rejectedState2 = false;
    bool rejectedState3 = false;
};
