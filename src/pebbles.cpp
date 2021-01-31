// Header
#include "pebbles.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"

#include <cmath>
#include <iostream>

#include "render_components.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

ECS::Entity Pebble::createPebble(vec2 position, vec2 scale)
{
	auto entity = ECS::Entity();

	std::string key = "pebble";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		// create a procedural circle
		std::vector<ColoredVertex> vertices;
		std::vector<uint16_t> indices;

		constexpr float z = -0.1f;
		constexpr int NUM_TRIANGLES = 62;

		for (int i = 0; i < NUM_TRIANGLES; i++) {
			// Point on the circle
			ColoredVertex v;
			v.position = { 0.5f * std::cos(float(PI) * 2.0f * static_cast<float>(i) / NUM_TRIANGLES),
						   0.5f * std::sin(float(PI) * 2.0f * static_cast<float>(i) / NUM_TRIANGLES),
						   z};
			v.color = { 0.8, 0.8, 0.8 };
			vertices.push_back(v);

			// Point on the circle ahead by on eposition in counter-clockwise direction
			v.position = { 0.5f * std::cos(float(PI) * 2.0f * static_cast<float>(i + 1) / NUM_TRIANGLES),
						   0.5f * std::sin(float(PI) * 2.0f * static_cast<float>(i + 1) / NUM_TRIANGLES),
						   z };
			v.color = { 0.8, 0.8, 0.8 };
			vertices.push_back(v);

			// Circle center
			v.position = { 0, 0, z };
			v.color = { 0.8, 0.8, 0.8 };
			vertices.push_back(v);

			// Indices
			// Note, one could create a mesh with less redundant vertices
			indices.push_back(static_cast<uint16_t>(i * 3 + 0));
			indices.push_back(static_cast<uint16_t>(i * 3 + 1));
			indices.push_back(static_cast<uint16_t>(i * 3 + 2));
		}
		RenderSystem::createColoredMesh(resource, "colored_mesh");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Create motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	motion.scale = scale;

	ECS::registry<Pebble>.emplace(entity);

	return entity;
}
