#include "parallax_background.hpp"
#include "render.hpp"

void BackgroundSystem::addBackground(std::string bgName, Parallax::Layer layer)
{
	auto entityLeft = ECS::Entity();
	auto entityRight = ECS::Entity();
	auto entityUp = ECS::Entity();
	auto entityDown = ECS::Entity();

	// Create the rendering components
	std::string key = bgName;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, backgrounds_path(bgName + ".png"), "textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entityLeft, resource, RenderBucket::BACKGROUND);
	ECS::registry<ShadedMeshRef>.emplace(entityRight, resource, RenderBucket::BACKGROUND);
	ECS::registry<ShadedMeshRef>.emplace(entityUp, resource, RenderBucket::BACKGROUND);
	ECS::registry<ShadedMeshRef>.emplace(entityDown, resource, RenderBucket::BACKGROUND);

	// Initialize the position, scale, and physics components
	float angle = 0.f;
	vec2 velocity = { 0, 0 };
	vec2 scale = static_cast<vec2>(resource.texture.size) * window_size_in_game_units.y / static_cast<vec2>(resource.texture.size).y;

	auto& motionLeft = ECS::registry<Motion>.emplace(entityLeft);
	auto& motionRight = ECS::registry<Motion>.emplace(entityRight);
	auto& motionUp = ECS::registry<Motion>.emplace(entityUp);
	auto& motionDown = ECS::registry<Motion>.emplace(entityDown);

	// using window_size for ypos since that's what the scale is based on
	motionLeft.angle = angle;
	motionLeft.velocity = velocity;
	motionLeft.scale = scale;
	motionLeft.position = { motionLeft.scale.x / 2, window_size_in_game_units.y / 2 };

	motionRight.angle = angle;
	motionRight.velocity = velocity;
	motionRight.scale = scale;
	motionRight.position = { motionRight.scale.x * 3 / 2, window_size_in_game_units.y / 2 };

	motionUp.angle = angle;
	motionUp.velocity = velocity;
	motionUp.scale = scale;
	motionUp.position = { motionUp.scale.x / 2, window_size_in_game_units.y * 3 / 2 };

	motionDown.angle = angle;
	motionDown.velocity = velocity;
	motionDown.scale = scale;
	motionDown.position = { motionDown.scale.x * 3 / 2, window_size_in_game_units.y * 3 / 2 };

	ECS::registry<Parallax>.emplace(entityLeft, layer);
	ECS::registry<Parallax>.emplace(entityRight, layer);
	ECS::registry<Parallax>.emplace(entityUp, layer);
	ECS::registry<Parallax>.emplace(entityDown, layer);

	backgrounds.emplace_back(std::make_pair(entityLeft, entityRight), std::make_pair(entityUp, entityDown));
}

void BackgroundSystem::addBackgrounds()
{
	removeBackgrounds();

	std::vector<Parallax::Layer> layers = { Parallax::LAYER_1, Parallax::LAYER_2, Parallax::LAYER_3, Parallax::LAYER_4, Parallax::LAYER_5,Parallax::LAYER_6, Parallax::LAYER_7, Parallax::LAYER_8 };
	std::vector<std::string> bgNames = { "1", "2", "3", "4", "5", "6", "7", "8" };
	for (int i = layers.size() - 1; i >= 0; i--)
		addBackground(bgNames[i], layers[i]);
}

void BackgroundSystem::removeBackgrounds()
{
	backgrounds.clear();

	for (ECS::Entity entity : ECS::registry<Parallax>.entities)
		ECS::ContainerInterface::remove_all_components_of(entity);
}

void BackgroundSystem::step()
{
	// if left/top side of background pair goes entirely offscreen,
	// move it to the right/bottom of the other texture to scroll infinitely

	// check all four backgrounds, since we don't know which is on the left/top
	for (auto& bgQuad : backgrounds)
	{

		auto& camera = ECS::registry<Camera>.entities[0];
		vec2 cameraOffset = ECS::registry<Motion>.get(camera).position;

		Motion& motionLeft = ECS::registry<Motion>.get(bgQuad.first.first);
		Motion& motionRight = ECS::registry<Motion>.get(bgQuad.first.second);
		Motion& motionUp = ECS::registry<Motion>.get(bgQuad.second.first);
		Motion& motionDown = ECS::registry<Motion>.get(bgQuad.second.second);

		// the backgrounds should all have the same layer (and scroll ratio)
		assert(ECS::registry<Parallax>.get(bgQuad.first.first).layer == ECS::registry<Parallax>.get(bgQuad.first.second).layer);
		assert(ECS::registry<Parallax>.get(bgQuad.first.first).layer == ECS::registry<Parallax>.get(bgQuad.second.first).layer);
		assert(ECS::registry<Parallax>.get(bgQuad.first.first).layer == ECS::registry<Parallax>.get(bgQuad.second.second).layer);
		
		Parallax& parallax = ECS::registry<Parallax>.get(bgQuad.first.first);
		float scrollRatio = 1 / static_cast<float>(parallax.layer);

		// walk to right
		if (motionLeft.position.x <= -motionLeft.scale.x / 2 + cameraOffset.x * scrollRatio)
			motionLeft.position.x = motionRight.position.x + motionRight.scale.x;
		else if (motionRight.position.x <= -motionRight.scale.x / 2 + cameraOffset.x * scrollRatio)
			motionRight.position.x = motionLeft.position.x + motionLeft.scale.x;

		// walk down
		if (motionUp.position.y <= -motionUp.scale.y / 2 + cameraOffset.y * scrollRatio)
			motionUp.position.y = motionDown.position.y + motionDown.scale.y;
		else if (motionDown.position.y <= -motionDown.scale.y / 2 + cameraOffset.y * scrollRatio)
			motionDown.position.y = motionUp.position.y + motionUp.scale.y;
	}
}

void BackgroundSystem::onNotify(Event event)
{
	switch (event.type)
	{
	case Event::LOAD_BG:
		addBackgrounds();
		break;
	case Event::CLOSE_BG:
		removeBackgrounds();
		break;
	}		
}
