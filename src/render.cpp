// internal
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "spider.hpp"
#include "snail.hpp"
#include "particle.hpp"
#include "render.hpp"
#include "text.hpp"

#include <iostream>

//change the data that gets passed to the fs on a frame-frame basis (nothing regarding intial loading here)
void DoSpriteSheetLogic(ECS::Entity entity, const ShadedMesh& mesh)
{
	auto& spriteSheet = ECS::registry<SpriteSheet>.get(entity);
	//we need to determine the vertOffset, horOffset
	auto width = mesh.texture.frameSize.x;
	auto height = mesh.texture.frameSize.y;
	auto vertOffset = height * spriteSheet.currentAnimationNumber;
	auto horOffset = width * spriteSheet.currentFrame; //when frame = 0, we get no offset.
	GLint vertOffset_uloc = glGetUniformLocation(mesh.effect.program, "vertOffset");
	GLint horOffset_uloc = glGetUniformLocation(mesh.effect.program, "horOffset");
	glUniform1f(vertOffset_uloc, vertOffset);
	glUniform1f(horOffset_uloc, horOffset);
}

void RenderSystem::HandleFrameSwitchTiming(ECS::Entity entity, float elapsed_ms)
{
	auto& spriteSheet = ECS::registry<SpriteSheet>.get(entity);
	spriteSheet.timeSinceFrameSwitch += elapsed_ms;
	if (spriteSheet.timeSinceFrameSwitch >= spriteSheet.animationSpeed)
	{
		spriteSheet.timeSinceFrameSwitch = 0;
		spriteSheet.currentFrame = (spriteSheet.currentFrame + 1) % spriteSheet.numAnimationFrames;
	}
}

void RenderSystem::drawTexturedMeshForParticles(ECS::Entity entity, vec2 window_size_in_game_units, const mat3& projection, float elapsed_ms)
{
    auto& motion = ECS::registry<Motion>.get(entity);
    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    
    Transform transform;
    transform.translate(motion.position);
    transform.scale({25, 25});
    
    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
    GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
    gl_has_errors();

    auto element = ECS::registry<WeatherParentParticle>.get(entity);
    std::vector<glm::vec2> translations;
    for (int y = 0; y < element.particles.size(); y++)
    {
        Motion& m = ECS::registry<Motion>.get(element.particles[y]);
        glm::vec2 translation;
        translation.x = (motion.position.x - m.position.x)/15.f;
        translation.y = abs(motion.position.y - m.position.y)/15.f;
        translations.push_back(translation);
    }
    
    if(translations.size() == 0) {
        return;
    }

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * element.particles.size(), &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    float quadVertices[30] = ;
    
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(randomBoolean ? quadVerticesSnow : quadVerticesRain), randomBoolean ? quadVerticesSnow : quadVerticesRain, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    // also set instance data
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.
    
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, element.particles.size()); // 100 triangles of 6 vertices each
    glBindVertexArray(0);

}

void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3& projection, float elapsed_ms)
{
	auto& motion = ECS::registry<Motion>.get(entity);
	auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
	// Transformation code, see Rendering and Transformation in the template specification for more info
	// Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(motion.position);
    transform.rotate(motion.angle);
	transform.scale(motion.scale);
	// !!! TODO A1: add rotation to the chain of transformations, mind the order of transformations

	// Setting shaders
	glUseProgram(texmesh.effect.program);
	glBindVertexArray(texmesh.mesh.vao);
	gl_has_errors();

	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	gl_has_errors();

	GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
	GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
	gl_has_errors();
    GLint time = glGetUniformLocation(texmesh.effect.program, "time");
    glUniform1f(time, static_cast<float>(glfwGetTime()));
    if(ECS::registry<Spider>.has(entity) && ECS::registry<DeathTimer>.has(entity) && texmesh.effect.geometry.resource!=0) {
        DeathTimer& dt = ECS::registry<DeathTimer>.get(entity);
        glUniform1f(time, static_cast<float>(10*Particle::timer - dt.counter_ms));
        float step_seconds = 1.0f * (elapsed_ms / 1000.f);
        GLint stepSeconds = glGetUniformLocation(texmesh.effect.program, "step_seconds");
        glUniform1f(stepSeconds, static_cast<float>(step_seconds));

        GLint centerPointX = glGetUniformLocation(texmesh.effect.program, "centerPointX");
        glUniform1f(centerPointX, static_cast<float>(motion.position.x ));
        GLint centerPointY = glGetUniformLocation(texmesh.effect.program, "centerPointY");
        glUniform1f(centerPointY, static_cast<float>(motion.position.y ));
    }
    
    gl_has_errors();
    
	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
	GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
	GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
	if (in_texcoord_loc >= 0)
	{
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); //
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
	}
	else if (in_color_loc >= 0)
	{
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));

		// Light up?
		// !!! TODO A1: check whether the entity has a LightUp component
		if (false)
		{
			GLint light_up_uloc = glGetUniformLocation(texmesh.effect.program, "light_up");

			// !!! TODO A1: set the light_up shader variable using glUniform1i
			(void)light_up_uloc; // placeholder to silence unused warning until implemented
		}
	}
	else
	{
		throw std::runtime_error("This type of entity is not yet supported");
	}
	gl_has_errors();

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
	glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);

    GLint alpha_uloc = glGetUniformLocation(texmesh.effect.program, "falpha");
	gl_has_errors();
    glUniform1f(alpha_uloc, texmesh.texture.alpha);
    gl_has_errors();

	if (ECS::registry<SpriteSheet>.has(entity)) 
	{

		HandleFrameSwitchTiming(entity, elapsed_ms);
		DoSpriteSheetLogic(entity, texmesh);
	}

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();
	GLsizei num_indices = size / sizeof(uint16_t);
	//GLsizei num_triangles = num_indices / 3;

	// Setting uniform values to the currently bound program
	glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);
}

// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawToScreen()
{
	// Setting shaders
	glUseProgram(screen_sprite.effect.program);
	glBindVertexArray(screen_sprite.mesh.vao);
	gl_has_errors();

	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(&window, &w, &h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();

	// Disable alpha channel for mapping the screen texture onto the real screen
	glDisable(GL_BLEND); // we have a single texture without transparency. Areas with alpha <1 cab arise around the texture transparency boundary, enabling blending would make them visible.
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, screen_sprite.mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen_sprite.mesh.ibo); // Note, GL_ELEMENT_ARRAY_BUFFER associates indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();

	// Draw the screen texture on the quad geometry
	gl_has_errors();

	// Set clock
	GLuint time_uloc = glGetUniformLocation(screen_sprite.effect.program, "time");
	GLuint dead_timer_uloc = glGetUniformLocation(screen_sprite.effect.program, "darken_screen_factor");
	glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
	auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
	glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
	gl_has_errors();

	// Set the vertex position and vertex texture coordinates (both stored in the same VBO)
	GLint in_position_loc = glGetAttribLocation(screen_sprite.effect.program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	GLint in_texcoord_loc = glGetAttribLocation(screen_sprite.effect.program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
	gl_has_errors();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen_sprite.texture.texture_id);

	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
	glBindVertexArray(0);
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw(vec2 window_size_in_game_units, float elapsed_ms)
{
	// Getting size of window
	ivec2 frame_buffer_size; // in pixels
	glfwGetFramebufferSize(&window, &frame_buffer_size.x, &frame_buffer_size.y);

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// Clearing backbuffer
	glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
	glDepthRange(0.00001, 10);
	glClearColor(0, 0, 1, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();

	auto& camera = ECS::registry<Camera>.entities[0];
	vec2 cameraOffset = ECS::registry<Motion>.get(camera).position;

	// projection that follows camera
	mat3 projection_2D = projection2D(window_size_in_game_units, cameraOffset);

	// stationary entities that don't move with camera
	mat3 overlay_projection_2D = projection2D(window_size_in_game_units, { 0, 0 });

	// Sort meshes for correct asset drawing order
	ECS::registry<ShadedMeshRef>.sort(renderCmp);

	// Draw all textured meshes that have a position and size component
	for (ECS::Entity entity : ECS::registry<ShadedMeshRef>.entities)
	{
		if (!ECS::registry<Motion>.has(entity))
			continue;
		
		// Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
		if (ECS::registry<Overlay>.has(entity))
			drawTexturedMesh(entity, overlay_projection_2D, elapsed_ms);
		else if (ECS::registry<Parallax>.has(entity))
			drawTexturedMesh(entity, projection2D(window_size_in_game_units, cameraOffset / static_cast<float>(ECS::registry<Parallax>.get(entity).layer)), elapsed_ms);
        else if (ECS::registry<WeatherParentParticle>.has(entity)) {
            drawTexturedMeshForParticles(entity, window_size_in_game_units, projection_2D, elapsed_ms);
        }
		else
			drawTexturedMesh(entity, projection_2D, elapsed_ms);

		gl_has_errors();
	}

	// Draw text components to the screen
	// NOTE: for simplicity, text components are drawn in a second pass,
	// on top of all texture mesh components. This should be reasonable
	// for nearly all use cases. If you need text to appear behind meshes,
	// consider using a depth buffer during rendering and adding a
	// Z-component or depth index to all rendererable components.
	for (const Text& text : ECS::registry<Text>.components) {
		drawText(text, window_size_in_game_units);
	}

	// Truely render to the screen
	drawToScreen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(&window);
}

mat3 RenderSystem::projection2D(vec2 window_size_in_game_units, vec2 offset)
{
	// Fake projection matrix, scales with respect to window coordinates
	float left = 0.f;
	float top = 0.f;
	float right = window_size_in_game_units.x;
	float bottom = window_size_in_game_units.y;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = (-(right + left) / (right - left)) - 2 * (offset.x / right);
	float ty = (-(top + bottom) / (top - bottom)) + 2 * (offset.y / bottom);
	
	return { { sx, 0.f, 0.f },{ 0.f, sy, 0.f },{ tx, ty, 1.f } };
}

void gl_has_errors()
{
	GLenum error = glGetError();

	if (error == GL_NO_ERROR)
		return;

	const char* error_str = "";
	while (error != GL_NO_ERROR)
	{
		switch (error)
		{
		case GL_INVALID_OPERATION:
			error_str = "INVALID_OPERATION";
			break;
		case GL_INVALID_ENUM:
			error_str = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error_str = "INVALID_VALUE";
			break;
		case GL_OUT_OF_MEMORY:
			error_str = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error_str = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}

		std::cerr << "OpenGL:" << error_str << std::endl;
		error = glGetError();
	}
	throw std::runtime_error("last OpenGL error:" + std::string(error_str));
}

bool renderCmp(ECS::Entity a, ECS::Entity b)
{
	RenderBucket a_bucket = ECS::registry<ShadedMeshRef>.get(a).renderBucket;
	RenderBucket b_bucket = ECS::registry<ShadedMeshRef>.get(b).renderBucket;

	return a_bucket > b_bucket;
}
bool RenderSystem::randomBoolean = RenderSystem::randomBool();
