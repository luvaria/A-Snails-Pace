#include "render_components.hpp"
#include "render.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb_image/stb_image.h"

// stlib
#include <array>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>

void gl_compile_shader(GLuint shader)
{
	glCompileShader(shader);
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> log(log_len);
		glGetShaderInfoLog(shader, log_len, &log_len, log.data());
		glDeleteShader(shader);

		throw std::runtime_error("GLSL: " + std::string(log.data()));
	}
}

// specialized destructors for all OpenGL resources that we support as of now
template<> GLResource<BUFFER>::~GLResource() noexcept{
	if (resource > 0)
		glDeleteBuffers(1, &resource);
}
template<> GLResource<VERTEX_ARRAY>::~GLResource() noexcept {
	if (resource > 0)
		glDeleteVertexArrays(1, &resource);
}
template<> GLResource<RENDER_BUFFER>::~GLResource() noexcept {
	if (resource > 0)
		glDeleteRenderbuffers(1, &resource);
}
template<> GLResource<TEXTURE>::~GLResource() noexcept {
	if (resource > 0)
		glDeleteTextures(1, &resource);
}
template<> GLResource<PROGRAM>::~GLResource() noexcept {
	if (resource > 0)
		glDeleteProgram(resource);
}
template<> GLResource<SHADER>::~GLResource() noexcept {
	if (resource > 0)
		glDeleteShader(resource);
}

void Texture::load_from_file(std::string path)
{
	stbi_uc* data;
	if (texture_cache.count(path) > 0)
		data = texture_cache[path];
	else
		data  = stbi_load(path.c_str(), &size.x, &size.y, NULL, 4);

	if (data == NULL)
		throw std::runtime_error("data == NULL, failed to load texture");
	gl_has_errors();

	glGenTextures(1, texture_id.data());
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	stbi_image_free(data);
	gl_has_errors();
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void Texture::create_from_screen(GLFWwindow const* window, GLuint* depth_render_buffer_id) {
	glGenTextures(1, texture_id.data());
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &size.x, &size.y);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Generate the render buffer with the depth buffer
	glGenRenderbuffers(1, depth_render_buffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, *depth_render_buffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *depth_render_buffer_id);

	// Set id as colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0);

	// Set the list of draw buffers
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("glCheckFramebufferStatus(GL_FRAMEBUFFER)");

	gl_has_errors();
}

bool Texture::is_valid() const
{
	return texture_id != 0;
}

void Effect::load_from_file(std::string vs_path, std::string fs_path)
{
	// Opening files
	std::ifstream vs_is(vs_path);
	std::ifstream fs_is(fs_path);
	if (!vs_is.good() || !fs_is.good())
		throw("Failed to load shader files " + vs_path +", "+ fs_path);

	// Reading sources
	std::stringstream vs_ss, fs_ss;
	vs_ss << vs_is.rdbuf();
	fs_ss << fs_is.rdbuf();
	std::string vs_str = vs_ss.str();
	std::string fs_str = fs_ss.str();
	const char* vs_src = vs_str.c_str();
	const char* fs_src = fs_str.c_str();
	GLsizei vs_len = (GLsizei)vs_str.size();
	GLsizei fs_len = (GLsizei)fs_str.size();

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vs_src, &vs_len);
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fs_src, &fs_len);

	// Compiling
	gl_compile_shader(vertex);
	gl_compile_shader(fragment);

	// Linking
	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	{
		GLint is_linked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
		if (is_linked == GL_FALSE)
		{
			GLint log_len;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
			std::vector<char> log(log_len);
			glGetProgramInfoLog(program, log_len, &log_len, log.data());

			throw std::runtime_error("Link error: "+ std::string(log.data()));
		}
	}
	gl_has_errors();
}

namespace {

    std::istream& consumeSlashSlash(std::istream& is){
        auto c = char{};
        for (int i = 0; i < 2; ++i) {
            is.get(c);
            if (!is || (c != '/')) {
                is.setstate(std::ios_base::failbit);
                return is;
            }
        }
        return is;
    };

} // anonymous namespace 

// Very, VERY simple OBJ loader adapted from https://github.com/opengl-tutorials/ogl tutorial 7
// (modified to also read vertex color and omit uv and normals)
void Mesh::loadFromOBJFile(std::string obj_path) {
    std::cout << "Loading OBJ file " << obj_path << std::endl;

	// Note: normal and UV indices are currently not used
	auto out_uv_indices = std::vector<uint16_t>{};
	auto out_normal_indices = std::vector<uint16_t>{};
	auto out_uvs = std::vector<glm::vec2>{};
	auto out_normals = std::vector<glm::vec3>{};

	// make sure we start from scratch
	this->vertices.clear();
	this->vertex_indices.clear();

	auto obj_file = std::ifstream{obj_path};
	if (!obj_file) {
		throw std::runtime_error("Could not open OBJ file " + obj_path);
	}

    auto line = std::string{};

    while (std::getline(obj_file, line)) {
        auto ss = std::istringstream{line};

        auto firstWord = std::string{};
        ss >> firstWord;

        if (!ss) {
            continue;
        }

        if (firstWord == "v") {
            auto vertex = ColoredVertex{};
            ss >> vertex.position.x >> vertex.position.y >> vertex.position.z;
            ss >> vertex.color.x >> vertex.color.y >> vertex.color.z;
            vertices.push_back(vertex);
        } else if (firstWord == "vt") {
            auto uv = glm::vec2{};
            ss >> uv.x >> uv.y;
            uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
            out_uvs.push_back(uv);
        } else if (firstWord == "vn") {
            auto normal = glm::vec3{};
            ss >> normal.x >> normal.y >> normal.z;
            out_normals.push_back(normal);
        } else if (firstWord == "f") {
            for (unsigned int i = 0; i < 3; ++i) {
                auto vi = std::uint16_t{};
                auto ni = std::uint16_t{};
                ss >> vi >> consumeSlashSlash >> ni;
                if (!ss) {
                    throw std::runtime_error("File can't be read by our simple parser :-( Try exporting with other options\n");
                }
                // -1 since .obj starts counting at 1 and OpenGL starts at 0
                assert(vi > 0);
                assert(ni > 0);
                --vi;
                --ni;
                this->vertex_indices.push_back(vi);
                out_normal_indices.push_back(ni);
            }
        } else {
            // Probably a comment
        }
    }

    // Compute bounds of the mesh
    constexpr auto float_min = std::numeric_limits<float>::min();
    constexpr auto float_max = std::numeric_limits<float>::max();

	auto max_position = vec3{float_min, float_min, float_min};
	auto min_position = vec3{float_max, float_max, float_max};
	for (auto& pos : vertices) {
		max_position = glm::max(max_position, pos.position);
		min_position = glm::min(min_position, pos.position);
	}

    // don't scale z direction
	min_position.z = 0;
	max_position.z = 1;

	auto extent = max_position - min_position;
	this->original_size = extent;

	// Normalize mesh to range -0.5 ... 0.5
	for (auto& pos : vertices) {
		pos.position = ((pos.position - min_position) / extent) - vec3(0.5f, 0.5f, 0.0f);
    }
}

// Returns a resource for every key, initializing with zero on the first query
ShadedMesh& cache_resource(std::string key) 
{
	static std::unordered_map<std::string, ShadedMesh> resource_cache;
	const auto it = resource_cache.find(key);
	if (it == resource_cache.end())
	{
		const auto it_succeeded = resource_cache.emplace(key, ShadedMesh{});
		assert(it_succeeded.second);
		return it_succeeded.first->second;
	}
	return it->second;
}

ShadedMeshRef::ShadedMeshRef(ShadedMesh& mesh) : 
	reference_to_cache(&mesh) 
{};