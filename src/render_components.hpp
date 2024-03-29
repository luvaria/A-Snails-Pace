#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

enum GLResourceType {BUFFER, RENDER_BUFFER, SHADER, PROGRAM, TEXTURE, VERTEX_ARRAY};

// This class is a wrapper around OpenGL resources that deletes allocated memory on destruction.
// Moreover, copy constructors are disabled to ensure that the resource is only deleted when the original object is destroyed, not its copies.
template <GLResourceType Resource>
struct GLResource
{
	GLuint resource = 0;

	///////////////////////////////////////////////////
	// Constructors and operators to mimick GLuint
	operator GLuint() const {
		return resource;
	};
	GLuint* data() {
		return &resource;
	};
	bool operator== (const GLuint& other) { return resource == other; }
	bool operator!= (const GLuint& other) { return resource != other; }
	bool operator<= (const GLuint& other) { return resource <= other; }
	bool operator>= (const GLuint& other) { return resource >= other; }
	bool operator< (const GLuint& other)  { return resource < other; }
	bool operator> (const GLuint& other) { return resource > other; }
	GLResource& operator= (const GLuint& other) { resource = other; return *this;}
	GLResource() = default;
	GLResource(const GLuint& other) {
		this->resource = other;
	};
	// Destructor that frees OpenGL resources, specializations for each supported type are in the .cpp
	~GLResource() noexcept;

	///////////////////////////////////////////////////
	// Operators that maintain exactly one indstance of every resources, e.g., OpenGL vertex buffer
	// Prevent copy
	GLResource(const GLResource& other) = delete; // copy constructor disabled
	GLResource& operator=(const GLResource&) = delete; // copy assignment disabled
	// Move constructor that invalidates the .bo in the source after copying
	GLResource(GLResource&& source) noexcept {
		// same as this->resource = source.resource; source.resource = 0;
		this->resource = std::exchange(source.resource, 0);
	};
	// Move operator that exchanges source.bo and this->bo, such that the this->bo lives on until destruction of the source
	GLResource& operator=(GLResource&& source) noexcept {
		// The exchange makes sure that the vbo and ibo of the target aren't overwritten in a move,
		// such that the destructor is called on every instanciated object exactly once. Leaving the 'other'
		// value unchanged would create a dublicated of it. Not exchanging the 'this' value would erase it without
		// release of associated resources (e.g., OpenGL buffers).
		if (this != &source)
			this->resource = std::exchange(source.resource, this->resource);
		return *this;
	};
};

// Single Vertex Buffer element for non-textured meshes (colored_mesh.vs.glsl & snail.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

struct Occluder 
{
	vec4 color = vec4((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)),
		(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)),
		(static_cast <float> (rand()) / static_cast <float> (RAND_MAX)),
		1); //alpha = 1
}; //entities which are solid and block light will have this component

// Texture wrapper
struct Texture
{
	GLResource<TEXTURE> texture_id;
	ivec2 size = {0, 0};
	vec2 frameSize = { 0,0 }; //<width, height> // set this when you load the sprite
	vec3 color = {1,1,1};
	float alpha = 1.0f; // only affects projectile for now, should it be implemented for all?
	
	// Loads texture from file specified by path
	void load_from_file(std::string path);
	bool is_valid() const; // True if texture is valid
	void create_from_screen(GLFWwindow const * const window, GLuint* depth_render_buffer_id); // Screen texture

	std::unordered_map<std::string, stbi_uc*> texture_cache;
};

// Effect component for Vertex and Fragment shader, which are then put(linked) together in a
// single program that is then bound to the pipeline.
struct Effect
{
	GLResource<SHADER> vertex;
	GLResource<SHADER> fragment;
    GLResource<SHADER> geometry;
	GLResource<PROGRAM> program;
    
    void load_from_file(std::string vs_path, std::string fs_path); // load shaders from files and link into program

	void load_from_file(std::string vs_path, std::string fs_path, std::string gs_path); // load shaders from files and link into program
    
    void load_from_file(std::string vs_path, std::string fs_path, std::string gs_path, bool withGeo);

};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	void loadFromOBJFile(std::string obj_path); //only use one of this function or the next one.
	void loadFromMinOBJFile(std::string obj_path);
	vec2 original_size = {1.f,1.f};
	GLResource<BUFFER> vbo;
	GLResource<BUFFER> ibo;
	GLResource<VERTEX_ARRAY> vao;
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

struct ScreenState
{
	float darken_screen_factor = -1;
};

struct ShadowScreen {};

// ShadedMesh datastructure for storing mesh, shader, and texture objects
struct ShadedMesh
{
	Mesh mesh;
	Effect effect;
	Texture texture;
};

// Cache for ShadedMesh resources (mesh consisting of vertex and index buffer, the vertex and fragment shaders, and the texture)
ShadedMesh& cache_resource(std::string key);

// Enum for rendering order layers/buckets
//	lower value = render at front
enum RenderBucket
{
	DEBUG = -4,
	OVERLAY_2 = -3,
	OVERLAY = -2,
	COLLECTIBLE = -1,
	PLAYER = 0,
	CHARACTER_COLLECTIBLE = 1,
	CHARACTER = 2,
	PROJECTILE = 3,
	TILE = 4,
	BACKGROUND = 5,
	BACKGROUND_2 = 6
};

// A wrapper that points to the ShadedMesh in the resource_cache
struct ShadedMeshRef
{
	ShadedMesh* reference_to_cache;
	RenderBucket renderBucket;
	ShadedMeshRef(ShadedMesh& mesh, RenderBucket bucket);
};

struct MinShadedMeshRef 
{
	ShadedMesh* reference_to_cache;
	RenderBucket renderBucket;
	MinShadedMeshRef(ShadedMesh& mesh, RenderBucket bucket);
};

struct Camera
{
	static void reset(vec2 position = { 0.f,0.f });
	static void update(float move_seconds);
	static vec2 getPosition();
	bool playSound = false;
	static bool shouldPlaySound();
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying snail
struct DeathTimer
{
	float counter_ms = 1500;
};

struct Parallax
{
	// Parallax depth layers
	enum Layer
	{
		// closest to camera
		LAYER_1 = 1,
		LAYER_2 = 2,
		LAYER_3 = 3,
		LAYER_4 = 4,
		LAYER_5 = 5,
		LAYER_6 = 6,
		LAYER_7 = 7,
		LAYER_8 = 8
		// furthest from camera
	};

	Layer layer = LAYER_1;
	Parallax(Layer l) : layer(l) {}
};
