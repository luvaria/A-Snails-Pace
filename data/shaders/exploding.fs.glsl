#version 330

uniform vec3 fcolor;
in vec3 vcolor;
//in vec2 TexCoords;

layout(location = 0) out vec4 color;

void main(void)
{
     color = vec4(fcolor * vcolor, 1.0);
}
