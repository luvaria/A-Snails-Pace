#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(0.0, 0.0, 0.0, 0.0);

    //now we step towards (0,0)

    //vec4 in_color = texture(screen_texture, texcoord);
    //bool check_colours = in_color.a != 0.0; //only check colours if the alpha is non-zero (starting on an object)

    vec2 unit_direction = normalize(vec2(-texcoord.x, 1.0 - texcoord.y));
    unit_direction.x *= 0.005;
    unit_direction.y *= 0.005;
    vec2 current_position = texcoord + unit_direction;
    while (current_position.x > 0.01 && current_position.y < 0.99) 
    {
        
        vec4 current_color = texture(screen_texture, current_position);
        if (current_color.a != 0.0) 
        {
            //(!check_colours || current_color != in_color)
            //if you are checking colours, and the color is the same, then it doesn't count

            vec4 after_color = texture(screen_texture, current_position + unit_direction / 2);
            if (after_color.a != 0.0)  
            {
                color = vec4(0.0, 0.0, 0.0, 0.2);
            }
        }
        current_position += unit_direction;
    }
}