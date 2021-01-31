// internal
#include "render.hpp"
#include "render_components.hpp"

#include <iostream>
#include <fstream>

// World initialization
RenderSystem::RenderSystem(GLFWwindow& window) :
	window(window)
{
	glfwMakeContextCurrent(&window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	gl3w_init();

	// Create a frame buffer
	frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	initScreenTexture();
}

RenderSystem::~RenderSystem()
{
	// delete allocated resources
	glDeleteFramebuffers(1, &frame_buffer);

	// remove all entities created by the render system
	while (ECS::registry<Motion>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());
	while (ECS::registry<ShadedMeshRef>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<ShadedMeshRef>.entities.back());
}

// Create a new sprite and register it with ECS
void RenderSystem::createSprite(ShadedMesh& sprite, std::string texture_path, std::string shader_name)
{
	if (texture_path.length() > 0)
		sprite.texture.load_from_file(texture_path.c_str());

	// The position corresponds to the center of the texture.
	TexturedVertex vertices[4];
	vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	vertices[0].texcoord = { 0.f, 1.f };
	vertices[1].texcoord = { 1.f, 1.f };
	vertices[2].texcoord = { 1.f, 0.f };
	vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	uint16_t indices[] = { 0, 3, 1, 1, 3, 2 };

	glGenVertexArrays(1, sprite.mesh.vao.data());
	glGenBuffers(1, sprite.mesh.vbo.data());
	glGenBuffers(1, sprite.mesh.ibo.data());
	gl_has_errors();

	// Vertex Buffer creation
	glBindBuffer(GL_ARRAY_BUFFER, sprite.mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // sizeof(TexturedVertex) * 4
	gl_has_errors();

	// Index Buffer creation
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // sizeof(uint16_t) * 6
	gl_has_errors();

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	// Loading shaders
	sprite.effect.load_from_file(shader_path(shader_name) + ".vs.glsl", shader_path(shader_name) + ".fs.glsl");
}

// Load a new mesh from disc and register it with ECS
void RenderSystem::createColoredMesh(ShadedMesh& texmesh, std::string shader_name)
{
	// Vertex Array
	glGenVertexArrays(1, texmesh.mesh.vao.data());
	glGenBuffers(1, texmesh.mesh.vbo.data());
	glGenBuffers(1, texmesh.mesh.ibo.data());
	glBindVertexArray(texmesh.mesh.vao);

	// Vertex Buffer creation
	glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredVertex) * texmesh.mesh.vertices.size(), texmesh.mesh.vertices.data(), GL_STATIC_DRAW);

	// Index Buffer creation
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * texmesh.mesh.vertex_indices.size(), texmesh.mesh.vertex_indices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	// Note, one could set vertex attributes here...
	// glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	// glEnableVertexAttribArray(0);
	// glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	// Loading shaders
	texmesh.effect.load_from_file(shader_path(shader_name)+".vs.glsl", shader_path(shader_name)+".fs.glsl");
}

// Initialize the screen texture from a standard sprite
void RenderSystem::initScreenTexture()
{
	// Create a sprite withour loading a texture
	createSprite(screen_sprite, "", "water");

	// Initialize the screen texture and its state
	screen_sprite.texture.create_from_screen(&window, depth_render_buffer_id.data());
	ECS::registry<ScreenState>.emplace(screen_state_entity);
}
