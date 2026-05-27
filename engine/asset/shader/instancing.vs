#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;
layout (location = 3) in ivec4 bone_ids;
layout (location = 4) in vec4 bone_weights;
layout (location = 5) in mat4 instance_matrix;
layout (location = 9) in vec4 instance_color;

uniform mat4 view;
uniform mat4 projection;

out vec4 color;

void main()
{
    color = instance_color;
    gl_Position = projection * view * instance_matrix * vec4(vertex_pos, 1.0f); 
}