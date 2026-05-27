#version 330 core
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 barycentric_coords;

void main()
{
    barycentric_coords.x = float(gl_VertexID % 3 == 0);
    barycentric_coords.y = float(gl_VertexID % 3 == 1);
    barycentric_coords.z = float(gl_VertexID % 3 == 2);

    gl_Position = projection * view * model * vec4(vertex_pos, 1.0);
}