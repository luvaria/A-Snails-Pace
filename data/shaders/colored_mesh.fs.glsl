#version 330

// From Vertex Shader
in vec3 vcolor;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	color = vec4(vcolor, 1.0);
}