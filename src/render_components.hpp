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

// Single Vertex Buffer element for non-textured meshes (colored_mesh.vs.glsl & salmon.vs.glsl)
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

// Texture wrapper
struct Texture
{
	GLResource<TEXTURE> texture_id;
	ivec2 size = {0, 0};
	vec3 color = {1,1,1};
	
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
	GLResource<PROGRAM> program;

	void load_from_file(std::string vs_path, std::string fs_path); // load shaders from files and link into program
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	void loadFromOBJFile(std::string obj_path);
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

// ShadedMesh datastructure for storing mesh, shader, and texture objects
struct ShadedMesh
{
	Mesh mesh;
	Effect effect;
	Texture texture;
};

// Cache for ShadedMesh resources (mesh consisting of vertex and index buffer, the vertex and fragment shaders, and the texture)
ShadedMesh& cache_resource(std::string key);

// A wrapper that points to the ShadedMesh in the resource_cache
struct ShadedMeshRef
{
	ShadedMesh* reference_to_cache;
	ShadedMeshRef(ShadedMesh& mesh);
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float counter_ms = 1000;
};
