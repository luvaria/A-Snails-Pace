#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float vertOffset;
uniform float horOffset;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec2 translatedTexcoord = vec2(texcoord.x + horOffset, texcoord.y + vertOffset);
	color = vec4(fcolor, 1.0) * texture(sampler0, vec2(translatedTexcoord.x, translatedTexcoord.y));
}
