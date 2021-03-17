// Header
#include "particle.hpp"
#include "render.hpp"
#include "tiles/tiles.hpp"
#include "common.hpp"
#include <random>

void Particle::setP1Motion(Motion& mot) {
    
    int minimum_number = -25;
    int max_number = -15;
    float xVel = rand() % (max_number + 1 - minimum_number) + minimum_number;
    minimum_number = 85;
    max_number = 105;
    float yVel = rand() % (max_number + 1 - minimum_number) + minimum_number;

    mot.angle = 0.2;
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
ECS::Entity Particle::createWeatherParticle(std::string particleName, ECS::Entity entity)
{
    // Create rendering primitives
    std::string key = particleName;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0) {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(key + ".png"), "textured", false);
    }
        // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::BACKGROUND);

//    resource.texture.alpha = 0.6f;
    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    //we use the same entity for min and regular meshes, so you can access either one.
    DeathTimer& dt = ECS::registry<DeathTimer>.emplace(entity);
    dt.counter_ms = WeatherParticle::timer;
    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    setP1Motion(motion);
    int minimum_number = -10;
    int max_number = 1600;
    float xValue = rand() % (max_number + 1 - minimum_number) + minimum_number;
    motion.position = { xValue , -10};

    // Create an (empty) Snail component
    ECS::registry<Particle>.emplace(entity);
    ECS::registry<WeatherParticle>.emplace(entity);

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
float WeatherParticle::timer = Particle::timer*50;
float WeatherParticle::nextSpawn = 2000;
float WeatherParticle::count = 1000;
