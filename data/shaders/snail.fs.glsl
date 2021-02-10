#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos; // Distance from local origin

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform int light_up;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	color = vec4(fcolor * vcolor, 1.0);

	// Snail mesh is contained in a 2x2 square (see snail.mesh for vertices)
	float radius = distance(vec2(0.0), vpos);
	if (light_up == 1 && radius < 0.3)
	{
		// 0.8 is just to make it not too strong
		color.xyz += (0.3 - radius) * 0.8 * vec3(1.0, 1.0, 1.0);
	}
}