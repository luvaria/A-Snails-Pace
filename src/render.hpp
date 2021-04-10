#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"

struct InstancedMesh;
struct ShadedMesh;

// OpenGL utilities
void gl_has_errors();

// Rendering order comparison function
bool renderCmp(ECS::Entity a, ECS::Entity b);

// System responsible for setting up OpenGL and for rendering all the 
// visual entities in the game
class RenderSystem
{
public:
	// Initialize the window
	RenderSystem(GLFWwindow& window);

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw(vec2 window_size_in_game_units, float elapsed_ms);

	// Expose the creating of visual representations to other systems
	static void createSprite(ShadedMesh& mesh_container, std::string texture_path, std::string shader_name, bool isSpriteSheet = false);
	static void createColoredMesh(ShadedMesh& mesh_container, std::string shader_name);

private:
	// Initialize the screeen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water shader
	void initScreenTexture();

	// Internal drawing functions for each entity type
	void drawTexturedMesh(ECS::Entity entity, const mat3& projection, float elapsed_ms, bool isOccluder);
	void drawToScreen();
	void drawShadowScreen();
	void HandleFrameSwitchTiming(ECS::Entity entity, float elapsed_ms);
    void drawTexturedMeshForParticles(ECS::Entity entity, vec2 window_size_in_game_units, const mat3& projection, float elapsed_ms);

	// Calculates 2D projection matrix based on offset
	mat3 projection2D(vec2 window_size_in_game_units, vec2 offset);

	// Window handle
	GLFWwindow& window;

	// Screen texture handles
	GLuint frame_buffer_2;
	GLuint frame_buffer;
	ShadedMesh screen_sprite;
	ShadedMesh shadow_sprite;
	GLResource<RENDER_BUFFER> depth_render_buffer_id;
	ECS::Entity screen_state_entity;
	ECS::Entity shadow_entity;
};
