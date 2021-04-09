#pragma once

// stlib
#include <string>
#include <tuple>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3
using namespace glm;
static const float PI = 3.14159265359f;

// Simple utility functions to avoid mistyping directory name
inline std::string data_path() { return "data"; };
inline std::string shader_path(const std::string& name) { return data_path() + "/shaders/" + name; };
inline std::string textures_path(const std::string& name) { return data_path() + "/textures/" + name; };
inline std::string audio_path(const std::string& name) { return data_path() + "/audio/" + name; };
inline std::string mesh_path(const std::string& name) { return data_path() + "/meshes/" + name; };
inline std::string levels_path(const std::string& name) { return data_path() + "/levels/" + name; };
inline std::string save_path(const std::string& name) { return data_path() + "/saves/" + name; };
inline std::string backgrounds_path(const std::string& name) { return data_path() + "/backgrounds/" + name; };
inline std::string dialogue_path(const std::string& name) { return data_path() + "/dialogue/" + name; };

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

#define DIRECTION_WEST  2
#define DIRECTION_NORTH 0
#define DIRECTION_SOUTH 1
#define DIRECTION_EAST  3

#define AI_PF_ALGO_BFS  "BFS"
#define AI_PF_ALGO_A_STAR "Astar"

// for use with levels_path(): use indices, starting from 0
const std::vector<std::string> levels = { "demo.json", "demo-2.json", "level-1.json", "level-2.json", "level-3.json", "level-4.json"};

const std::unordered_map<int, std::string> collectibleMap =
{
    { 0, "tophat" },
	{ 1, "bbcap" }
};

struct Player
{
	// tag component
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	float angle = 0;
	vec2 velocity = { 0, 0 };
	vec2 scale = { 10, 10 };
	int lastDirection = DIRECTION_NORTH;
};

struct Fire {
	bool fired;
};

struct SpriteSheet 
{
	int numAnimationFrames = 1;
	float animationSpeed = 10000;
	int currentAnimationNumber = 0;
	int currentFrame = 0;
	float timeSinceFrameSwitch = 0;
};

struct Destination {
	vec2 position = { 0, 0 };
};

// renders independent of camera offset
struct Overlay
{
	// empty; tag component
};

enum TurnType { PLAYER_WAITING, NPC_ENCOUNTER, PLAYER_UPDATE, ENEMY, CAMERA };

struct Turn
{
    TurnType type;
};

struct Enemy
{
	// tag component for all enemies
};

typedef int CollectId;
struct Inventory
{
    std::unordered_set<CollectId> collectibles;
    CollectId equipped = -1;
    int points = 0;
    void clear() { collectibles.clear(); equipped = -1; points = 0; };
};

// collectible equipped; ignore collisions
struct NoCollide {};

struct DirectionInput
{
	int direction; //using same direction values as motion.last_direction
};

struct CornerMotion 
{
	float t = 0; // t between 0 and 1 for rounding the corner.
	float total_time = 0; //total elapsed_time that it should take to round the corner
	float theta_old = 0; //represents the angle that the previous time step had
	float segment_angle = 0; //angle for the previous segment
	float dest_angle = 0;
	int numSegmentsCompleted = 0;
	int numSegments = 0;
	bool isSegmentAngleIncreasing = true;
	//coefficients of the basis functions for stepping around the corner
	vec2 P0;
	vec2 P1;
	vec2 T0;
	vec2 T1;
};

// direction a level scrolls in
enum ScrollDirection
{
	LEFT_TO_RIGHT,
	TOP_TO_BOTTOM
};
