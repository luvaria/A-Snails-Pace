#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include <random>
#include <functional>

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
    
    static bool randomBool() {
        static auto gen = std::bind(std::uniform_int_distribution<>(0,1),std::default_random_engine());
        bool b = gen();
        return b;
    }
    
    static bool randomBoolean; 

private:
    unsigned int instanceVBO = 0;
    unsigned int quadVAO = 0, quadVBO = 0;

    float quadVerticesSnow[30] = {
            -0.05f,  0.05f,  1.0f, 1.0f, 1.0f,
             0.05f, -0.05f,  1.0f, 1.0f, 1.0f,
            -0.05f, -0.05f,  1.0f, 1.0f, 1.0f,

            -0.05f,  0.05f,  1.0f, 1.0f, 1.0f,
             0.05f, -0.05f,  1.0f, 1.0f, 1.0f,
             0.05f,  0.05f,  1.0f, 1.0f, 1.0f
    };
    
    float quadVerticesRain[30] = {
        -0.05f,  0.05f,  0.447f, 0.737f, 0.831f,
         0.05f, -0.05f,  0.447f, 0.737f, 0.831f,
        -0.05f, -0.05f,  0.447f, 0.737f, 0.831f,

        -0.05f,  0.05f,  0.447f, 0.737f, 0.831f,
         0.05f, -0.05f,  0.447f, 0.737f, 0.831f,
         0.05f,  0.05f,  0.447f, 0.737f, 0.831f
    };
    
    
    
	// Initialize the screeen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water shader
	void initScreenTexture();

	// Internal drawing functions for each entity type
	void drawTexturedMesh(ECS::Entity entity, const mat3& projection, float elapsed_ms);
	void drawToScreen();
	void HandleFrameSwitchTiming(ECS::Entity entity, float elapsed_ms);
    void drawTexturedMeshForParticles(ECS::Entity entity, vec2 window_size_in_game_units, const mat3& projection, float elapsed_ms);

	// Calculates 2D projection matrix based on offset
	mat3 projection2D(vec2 window_size_in_game_units, vec2 offset);

	// Window handle
	GLFWwindow& window;

	// Screen texture handles
	GLuint frame_buffer;
	ShadedMesh screen_sprite;
	GLResource<RENDER_BUFFER> depth_render_buffer_id;
	ECS::Entity screen_state_entity;
};
