#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float time;

// Output color
layout(location = 0) out  vec4 color;

const float PI = 3.14159265359;

// sky colours
const vec3 DEEP_BLUE = vec3(0.0, 0.5, 0.8);
const vec3 LIGHT_BLUE = vec3(0.8, 0.9, 1.0);
const vec3 WHITE = vec3(1.0, 1.0, 1.0);
const vec3 YELLOW = vec3(1.0, 0.8, 0.6);
const vec3 BLUE_GREY = vec3(0.90, 0.95, 0.975);
const vec3 GREY = vec3(0.95, 0.95, 0.95);
const vec3 BLACK = vec3(0.0, 0.0, 0.0);

// stars
const vec2 TILE_SIZE = vec2(0.03, 0.03); // control maximum size of stars (and their spacing)
const float STAR_DENSITY = 0.3; // control how many stars
const float STAR_DENSITY_DISTR = 1.5; // control how much density changes based on distance from sun
const float STAR_ROTATION = -0.05; // control how quickly the stars move across the sky
const float TAIL_ROTATION = 0.03; // cannot be too large due to tiling

// sun position
// stars rotate around the sun because we're already ignoring physics by having both
const vec2 sun_uv = vec2(0.7, 0.8);


// hash from: https://www.shadertoy.com/view/4djSRW
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 rotate_about_sun(vec2 uv_in, float angle)
{
    vec2 uv_out = vec2(0);

    // formula to rotate point about another point from: https://stackoverflow.com/a/15109215
    uv_out.x = cos(angle) * (uv_in.x - sun_uv.x) - sin(angle) * (uv_in.y - sun_uv.y) + sun_uv.x;
    uv_out.y = sin(angle) * (uv_in.x - sun_uv.x) + cos(angle) * (uv_in.y - sun_uv.y) + sun_uv.y;
    
    return uv_out;
}

void sky(inout vec3 col, float sun_dist)
{
    col = mix(LIGHT_BLUE, DEEP_BLUE, sun_dist);
}

// tiled glowing effect inspired in part by https://www.shadertoy.com/view/WldyRf
void stars(inout vec3 col, vec2 star_uv)
{
    // determine which tile the fragment is in
    // this is slightly awkward as sometimes stars appear in an obvious tiled pattern
    vec2 tile_uv = vec2(0);
    tile_uv.x = floor(star_uv.x / TILE_SIZE.x) * TILE_SIZE.x + 0.5 * TILE_SIZE.x;
    tile_uv.y = floor(star_uv.y / TILE_SIZE.y) * TILE_SIZE.y + 0.5 * TILE_SIZE.y;

    // rand based on tile pos
    float tile_rand = hash12(tile_uv);

    // tile dist from sun
    float tile_dist = distance(tile_uv, sun_uv);

    // draw star based on rand, higher density away from sun
    if (tile_rand <= STAR_DENSITY * pow(tile_dist, STAR_DENSITY_DISTR))
    {
        // distance of fragment from centre of tile
        float max_dist = length(vec2(0.5 * TILE_SIZE.x, 0.5 * TILE_SIZE.y));
        float star_dist = distance(star_uv, tile_uv);

        // radius of star based on time and tile pos
        float max_radius = 0.22 * TILE_SIZE.x * (tile_rand / STAR_DENSITY);
        float radius = max_radius * (0.5 + 0.5 * sin(100000.0 * tile_rand / STAR_DENSITY + time));

        col += WHITE * smoothstep(radius, 0.0, star_dist);

        // tail/afterimage
        vec2 tail_uv = rotate_about_sun(star_uv, TAIL_ROTATION * star_dist / max_dist);
        float tail_dist = distance(tail_uv, tile_uv);
        col += DEEP_BLUE * 0.3 * smoothstep(radius, 0.0, tail_dist);
    }
}

void sun (inout vec3 col, float sun_dist)
{
    // fluctuating yellow glow
    col = mix(YELLOW, col, clamp(sun_dist * 3.0 / (0.7 + 0.3*abs(sin(0.8*time))), 0.0, 1.0));
    // white centre
    col = mix(col, WHITE, step(sun_dist, 0.08));
}

void ground (inout vec3 col, float ground_offset)
{
    if (ground_offset >= 0.0)
    {
        col = mix(BLUE_GREY, GREY, ground_offset/(1.0-sun_uv.y));    
    }   
}

void main()
{
    vec2 uv = texcoord;

    // initial fragment colour
    vec3 col = vec3(0);
    
    // distance from sun
    float sun_dist = clamp(distance(uv, sun_uv), 0.0, 1.0);

    // sky
    sky(col, sun_dist);
    
    // stars
    vec2 star_uv = rotate_about_sun(uv, time * STAR_ROTATION * PI);
    stars(col, star_uv);

    // sun
    sun(col, sun_dist);

    // ground
    float ground_offset = uv.y - sun_uv.y;
    ground(col, ground_offset); 

    // output to screen
    color = vec4(col,1.0);
}
