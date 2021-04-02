#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

out vec3 fColor;
uniform mat3 projection;
uniform mat3 transform;

void main()
{
    fColor = aColor;
    vec3 pos = projection * transform * vec3(aPos.xy + aOffset.xy, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
}
