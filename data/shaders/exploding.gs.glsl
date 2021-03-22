#version 330
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec3 colors[];
out vec3 vcolor;
uniform float time;
uniform float centerPointX;
uniform float centerPointY;
uniform float step_seconds;
uniform mat3 transform;
uniform mat3 projection;

vec3 GetNormal()
{
   float centerX = (gl_in[0].gl_Position.x + gl_in[1].gl_Position.x + gl_in[2].gl_Position.x) / 3;
   float centerY = (gl_in[0].gl_Position.y + gl_in[1].gl_Position.y + gl_in[2].gl_Position.y) / 3;

    
    vec3 norm = normalize(- (vec3(centerPointX, centerPointY, 1.0)) + inverse(projection) * vec3(centerX, centerY, 1.0));
    return vec3(norm.xy, gl_in[1].gl_Position.z);
}

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 0.0008;
    vec3 direction = normal * time * magnitude;
    return vec4(position.xy + direction.xy, position.z, position.w);
//    normal = normal.xyz * (0.001 * time) + 0.1;
//    return position + vec4(normal.xyz , 0.0);
}

void main()
{
  vec3 normal = GetNormal();

  for(int i=0; i<3; i++)
  {
    gl_Position = explode(gl_in[i].gl_Position, normal);
    vcolor = colors[i];
    EmitVertex();
  }
  EndPrimitive();
}  
