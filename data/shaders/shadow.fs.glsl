#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(0.0, 0.0, 0.0, 0.0);

    //now we step towards (0.5,1)

    vec2 unit_direction = normalize(vec2(0.5 - texcoord.x, 1.0 - texcoord.y));
    unit_direction.x *= 0.005;
    unit_direction.y *= 0.005;
    vec2 current_position = texcoord + unit_direction;
    while (current_position.x > 0.001 && current_position.x < 0.999 && current_position.y < 0.999) 
    {
        
        vec4 current_color = texture(screen_texture, current_position);
        if (current_color.a != 0.0) 
        {
            color = vec4(0.0, 0.0, 0.0, 0.2);
        }
        current_position += unit_direction;
    }
}