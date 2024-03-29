// Header
#include "particle.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "common.hpp"
#include <random>

std::default_random_engine rng;
std::uniform_real_distribution<float> uniform_dist;

void Particle::setP1Motion(Motion& mot) {
    
    int minimum_number = -25;
    int max_number = -15;
    float xVel = rand() % (max_number + 1 - minimum_number) + minimum_number;
    minimum_number = 85;
    max_number = 105;
    float yVel = rand() % (max_number + 1 - minimum_number) + minimum_number;

    mot.angle = 0.2f;
    mot.lastDirection = DIRECTION_WEST;
    mot.scale = {12, 12};
    mot.velocity = {xVel, yVel};
}

void Particle::setP2Motion(Motion& mot) {
    mot.angle = -mot.angle;
    mot.lastDirection = DIRECTION_EAST;
    int minimum_number = +15;
    int max_number = +5;
    float xVel = rand() % (max_number + 1 - minimum_number) + minimum_number;
    minimum_number = 75;
    max_number = 92;
    float yVel = rand() % (max_number + 1 - minimum_number) + minimum_number;

  
    mot.velocity = {xVel, yVel};
}

void Particle::setP3Motion(Motion& mot) {
    mot.angle = 0;
    mot.lastDirection = DIRECTION_EAST;
    int minimum_number = -5;
    int max_number = +5;
    float xVel = rand() % (max_number + 1 - minimum_number) + minimum_number;
    minimum_number = 65;
    max_number = 85;
    float yVel = rand() % (max_number + 1 - minimum_number) + minimum_number;

    mot.velocity = {xVel, yVel};
}

// Snowflake credits: https://pngio.com/PNG/a44119-png-snowflake.html
// Direct link: https://img.pngio.com/snowflakes-png-images-free-download-snowflake-png-png-snowflake-2500_2500.png
ECS::Entity Particle::createWeatherParticle(std::string particleName, ECS::Entity entity, vec2 window_size_in_game_units)
{
    // Create rendering primitives
    std::string key = particleName;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0) {
        resource = ShadedMesh();
        std::string shader_name="weatherparticle";
        resource.effect.load_from_file(shader_path(shader_name) + ".vs.glsl", shader_path(shader_name) + ".fs.glsl");
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::BACKGROUND);
    DeathTimer& dt = ECS::registry<DeathTimer>.emplace(entity);
    dt.counter_ms = WeatherParentParticle::timer;
    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    setP1Motion(motion);
    
    assert(ECS::registry<Camera>.size() != 0);
    auto &cameraEntity = ECS::registry<Camera>.entities[0];

    assert(ECS::registry<Motion>.has(cameraEntity));
    auto &cameraMotion = ECS::registry<Motion>.get(cameraEntity);
    
    int minimum_number = cameraMotion.position.x - 10;
    int max_number = cameraMotion.position.x + window_size_in_game_units.x + 200;
    float xValue = cameraMotion.position.x + (window_size_in_game_units.x) / 2;
    
    minimum_number = cameraMotion.position.y - 200;
    max_number = cameraMotion.position.y - 100;
    float yValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
    
    motion.position = { xValue , yValue};
    ECS::registry<Particle>.emplace(entity);
    ECS::registry<WeatherParentParticle>.emplace(entity);
//    ECS::registry<WeatherParticle>.emplace(entity);
//    ECS::registry<RejectedStages>.emplace(entity);
//
//    int index = 0;
//    auto& ele = ECS::registry<WeatherParentParticle>.get(entity);
//    while (index < WeatherParticle::count)
//    {
//        ECS::Entity newSnowflakeParticle;
//        Particle::createWeatherChildParticle("snowflake", newSnowflakeParticle, window_size_in_game_units);
//        ele.particles[index++] = newSnowflakeParticle;
//    }
    return entity;
}

ECS::Entity Particle::createWeatherChildParticle(std::string particleName, ECS::Entity entity, ECS::Entity parentEntity, vec2 window_size_in_game_units)
{
    std::string key = particleName;
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    setP1Motion(motion);
    
    DeathTimer& dt = ECS::registry<DeathTimer>.emplace(entity);
    dt.counter_ms = WeatherParentParticle::timer;
    
    assert(ECS::registry<Camera>.size() != 0);
    auto &cameraEntity = ECS::registry<Camera>.entities[0];

    assert(ECS::registry<Motion>.has(cameraEntity));
    auto &cameraMotion = ECS::registry<Motion>.get(cameraEntity);
    
    int minimum_number = cameraMotion.position.x - 10;
    int max_number = cameraMotion.position.x + window_size_in_game_units.x + 200;
    float xValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
    
    minimum_number = cameraMotion.position.y - 100;
    max_number = cameraMotion.position.y - 1;
    float yValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
    
    motion.position = { xValue , yValue};
    ECS::registry<Particle>.emplace(entity);
    auto& wp = ECS::registry<WeatherParticle>.emplace(entity);
    wp.parentEntity = parentEntity;
    ECS::registry<RejectedStages>.emplace(entity);
    return entity;
}

ECS::Entity Particle::createParticle(Motion providedMotion, ECS::Entity entity)
{
    std::string key = "particle";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("snail.obj"));
        RenderSystem::createColoredMesh(resource, "particle");
    }

    std::string key_min = "minParticle";
    ShadedMesh& resource_min = cache_resource(key_min);
    if (resource_min.mesh.vertices.size() == 0)
    {
        resource_min.mesh.loadFromMinOBJFile(mesh_path("snail-min.obj"));
    }
    resource.texture.alpha = 0.3f;
    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::PROJECTILE);
    ECS::registry<MinShadedMeshRef>.emplace(entity, resource_min, RenderBucket::PROJECTILE);
    DeathTimer& dt = ECS::registry<DeathTimer>.emplace(entity);
    dt.counter_ms = BlurParticle::timer;
    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position =  providedMotion.position;
    motion.angle = providedMotion.angle;
    if(providedMotion.lastDirection == DIRECTION_NORTH) {
        motion.velocity = { -0.3, 4.f };
    } else if (providedMotion.lastDirection == DIRECTION_SOUTH) {
        motion.velocity = { -0.3, -4.f };
    } else if (providedMotion.lastDirection == DIRECTION_WEST) {
        motion.velocity = { +4.f, -0.3 };
    } else {
        motion.velocity = { -4.f, -0.3 };
    }
    motion.scale =  providedMotion.scale;
    motion.scale = providedMotion.scale;
    motion.lastDirection = providedMotion.lastDirection;

    // Create an (empty) Snail component
    ECS::registry<Particle>.emplace(entity);
    ECS::registry<BlurParticle>.emplace(entity);

    return entity;
}
float Particle::timer = 150;
bool BlurParticle::canCreateNew = true;
float BlurParticle::timer = Particle::timer;
float WeatherParentParticle::timer = Particle::timer*40;
float WeatherParentParticle::nextSpawn = 2000;
float WeatherParentParticle::count = 10;
float WeatherParticle::nextSpawn = 1000;
int WeatherParticle::count = 300;
